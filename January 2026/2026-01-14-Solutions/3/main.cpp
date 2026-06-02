#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include "include/DES.hpp"
#include "include/IO.hpp"
#include "include/Random.hpp"
#include "include/Stat.hpp"

namespace SELib {

class CustomerProcess : public DiscreteEventProcess {
public:
    CustomerProcess(int pid, int S, int P, int Q, double A, double B, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), S_(S), P_(P), Q_(Q), A_(A), B_(B), bus_(bus), rng_(rng) {}
    
    
    void initialize(double startTime) override { 
        nextTime_ = startTime + rng_.uniform(A_, B_); 
    }

    double nextEventTime() const override {
        return nextTime_;
    }

    void handleEvent(double currentTime) override {
        Message m;
        m.time = currentTime;
        m.sender = pid_;
        m.receiver = 100 + rng_.uniformInt(0, S_ - 1);
        m.item = rng_.uniformInt(1, P_);
        m.quantity = (double)rng_.uniformInt(1, Q_);
        bus_.procToNet(pid_).push(m);
        nextTime_ = currentTime + rng_.uniform(A_, B_);
    }

private:
    int pid_, S_, P_, Q_;
    double A_, B_, nextTime_;
    MessageBus &bus_;
    RandomGenerator &rng_;
};

class SupplierProcess : public DiscreteEventProcess {
public:
    SupplierProcess(int pid, int S, int P, int Q, double V, double W, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), S_(S), P_(P), Q_(Q), V_(V), W_(W), bus_(bus), rng_(rng) {}
    void initialize(double startTime) override { nextTime_ = startTime + rng_.uniform(V_, W_); }
    double nextEventTime() const override { return nextTime_; }
    void handleEvent(double currentTime) override {
        Message m;
        m.time = currentTime; m.sender = pid_; m.receiver = 100 + rng_.uniformInt(0, S_ - 1);
        m.item = rng_.uniformInt(1, P_); m.quantity = (double)rng_.uniformInt(0, Q_);
        bus_.procToNet(pid_).push(m);
        nextTime_ = currentTime + rng_.uniform(V_, W_);
    }
private:
    int pid_, S_, P_, Q_; double V_, W_, nextTime_; MessageBus &bus_; RandomGenerator &rng_;
};

class ServerProcess : public DiscreteEventProcess {
public:
    ServerProcess(int pid, int C, int F, int P, int Q, MessageBus &bus, RandomGenerator &rng, long long &ms)
        : pid_(pid), C_(C), F_(F), P_(P), Q_(Q), bus_(bus), rng_(rng), ms_(ms) {
        inv_.assign(P + 1, 0);
    }
    void initialize(double startTime) override {
        for(int i=1; i<=P_; ++i) inv_[i] = rng_.uniformInt(0, Q_);
        nextTime_ = startTime + 0.1;
    }
    double nextEventTime() const override { return nextTime_; }
    void handleEvent(double currentTime) override {
        auto &inbox = bus_.netToProc(pid_);
        while (!inbox.empty()) {
            Message m = inbox.front(); inbox.pop();
            bool isSupply = (m.sender >= C_ && m.sender < C_ + F_);
            if (isSupply) {
                inv_[m.item] += (int)m.quantity;
            } else {
                int req = (int)m.quantity;
                if (inv_[m.item] < req) ms_++;
                int prov = std::min(req, inv_[m.item]);
                inv_[m.item] -= prov;
            }
        }
        nextTime_ = currentTime + 0.1; // Periodic Polling
    }
private:
    int pid_, C_, F_, P_, Q_; MessageBus &bus_; RandomGenerator &rng_; long long &ms_;
    double nextTime_; std::vector<int> inv_;
};

}

int main() {
    SELib::RandomGenerator rng(0);
    SELib::ParameterParser parser;
    if (!parser.parseFile("parameters.txt")) return 1;
    double H = parser.getDouble("H");
    int M = parser.getInt("M"), C = parser.getInt("C"), S = parser.getInt("S"), P = parser.getInt("P"), F_suppliers = parser.getInt("F"), Q = parser.getInt("Q");
    double A = parser.getDouble("A"), B = parser.getDouble("B"), V = parser.getDouble("V"), W = parser.getDouble("W");

    SELib::Statistics stats;
    for (int m = 0; m < M; ++m) {

        // Init message bus
        SELib::MessageBus bus(200);

        // Init DES system
        SELib::DiscreteEventSystem system;
        long long ms = 0;

        // Create processes: C customers, F suppliers, S servers, 1 router
        for(int i=0; i<C; ++i) {
            system.emplaceProcess<SELib::CustomerProcess>(i, S, P, Q, A, B, bus, rng);
        }
        for(int i=0; i<F_suppliers; ++i) {
            system.emplaceProcess<SELib::SupplierProcess>(C+i, S, P, Q, V, W, bus, rng);
        }
        for(int i=0; i<S; ++i) {
            system.emplaceProcess<SELib::ServerProcess>(100+i, C, F_suppliers, P, Q, bus, rng, ms);
        }
        system.emplaceProcess<SELib::NetworkRouter>(bus, rng, 0.01);

        // Init DES simulator 
        SELib::DiscreteEventSimulator sim(system);

        // Run simulation
        sim.run(H);

        stats.addSample((double)ms / H);
    }
    std::ofstream outFile("results.txt");
    outFile << "2026-01-14-Axel-Rubini-2158099" << std::endl;
    outFile << "R " << std::fixed << std::setprecision(6) << stats.mean() << std::endl;
    return 0;
}