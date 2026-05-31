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
#include "include/Decision.hpp"

namespace SELib {

class CustomerProcess : public DiscreteEventProcess {
public:
    CustomerProcess(int pid, int S, int P, int Q, double A, double B, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), S_(S), P_(P), Q_(Q), A_(A), B_(B), bus_(bus), rng_(rng) {}
    void initialize(double startTime) override { nextTime_ = startTime + rng_.uniform(A_, B_); }
    double nextEventTime() const override { return nextTime_; }
    void handleEvent(double currentTime) override {
        Message m; m.time = currentTime; m.sender = pid_; m.receiver = 100 + rng_.uniformInt(0, S_ - 1);
        m.item = rng_.uniformInt(1, P_); m.quantity = (double)rng_.uniformInt(1, Q_);
        bus_.procToNet(pid_).push(m);
        nextTime_ = currentTime + rng_.uniform(A_, B_);
    }
private:
    int pid_, S_, P_, Q_; double A_, B_, nextTime_; MessageBus &bus_; RandomGenerator &rng_;
};

class SupplierProcess : public DiscreteEventProcess {
public:
    SupplierProcess(int pid, int S, int P, int Q, double V, double W, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), S_(S), P_(P), Q_(Q), V_(V), W_(W), bus_(bus), rng_(rng) {}
    void initialize(double startTime) override { nextTime_ = startTime + rng_.uniform(V_, W_); }
    double nextEventTime() const override { return nextTime_; }
    void handleEvent(double currentTime) override {
        Message m; m.time = currentTime; m.sender = pid_; m.receiver = 100 + rng_.uniformInt(0, S_ - 1);
        m.item = -rng_.uniformInt(1, P_); m.quantity = (double)rng_.uniformInt(0, Q_);
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
            bool isSupply = (m.item < 0);
            if (isSupply) { inv_[-m.item] += (int)m.quantity; }
            else {
                int req = (int)m.quantity;
                if (inv_[m.item] < req) ms_++;
                int prov = std::min(req, inv_[m.item]); inv_[m.item] -= prov;
            }
        }
        nextTime_ = currentTime + 0.1;
    }
private:
    int pid_, C_, F_, P_, Q_; MessageBus &bus_; RandomGenerator &rng_; long long &ms_;
    double nextTime_; std::vector<int> inv_;
};

struct Result { double R, V, W, J; };

Result runMsims(double V_val, double H, int M, int C, int S, int P, int Q, int F_suppliers, double A, double B, RandomGenerator &rng, double a, double b) {
    Statistics s; double W_val = V_val + 5.0;
    for (int m = 0; m < M; ++m) {
        MessageBus bus(200); DiscreteEventSystem system; long long ms = 0;
        for(int i=0; i<C; ++i) system.emplaceProcess<CustomerProcess>(i, S, P, Q, A, B, bus, rng);
        for(int i=0; i<F_suppliers; ++i) system.emplaceProcess<SupplierProcess>(C+i, S, P, Q, V_val, W_val, bus, rng);
        for(int i=0; i<S; ++i) system.emplaceProcess<ServerProcess>(100+i, C, F_suppliers, P, Q, bus, rng, ms);
        system.emplaceProcess<NetworkRouter>(bus, rng, 0.01);
        DiscreteEventSimulator sim(system); sim.run(H);
        s.addSample((double)ms / H);
    }
    double R_mean = s.mean(); return {R_mean, V_val, W_val, a * V_val + b * R_mean};
}

}

int main() {
    SELib::RandomGenerator rng(0); SELib::ParameterParser parser; if (!parser.parseFile("parameters.txt")) return 1;
    double H = parser.getDouble("H");
    int M = parser.getInt("M"), C = parser.getInt("C"), S = parser.getInt("S"), P = parser.getInt("P"), Q = parser.getInt("Q"), F_suppliers = parser.getInt("F"), G = parser.getInt("G");
    double A = parser.getDouble("A"), B = parser.getDouble("B"), a_cost = parser.getDouble("a"), b_cost = parser.getDouble("b");

    SELib::Result bestRes = {0, 0, 0, 1e18};
    for (int i = 0; i < G; ++i) {
        double V_cand = rng.uniform(0.1, 10.0);
        auto res = SELib::runMsims(V_cand, H, M, C, S, P, Q, F_suppliers, A, B, rng, a_cost, b_cost);
        if (res.J < bestRes.J) bestRes = res;
    }
    std::ofstream outFile("results.txt"); outFile << "2026-01-14-Axel-Rubini-2158099" << std::endl;
    outFile << "R " << bestRes.R << std::endl; outFile << "V " << bestRes.V << std::endl; outFile << "W " << bestRes.W << std::endl; outFile << "J " << bestRes.J << std::endl;
    return 0;
}
