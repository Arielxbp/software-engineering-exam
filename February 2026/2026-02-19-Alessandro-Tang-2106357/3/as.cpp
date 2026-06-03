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

class DatabaseProcess : public DiscreteEventProcess {
public:
    DatabaseProcess(int pid, int supp_start, int F, int P, int Q, double l, double s, MessageBus &bus, RandomGenerator &rng, long long &ms, long long &tx)
        : pid_(pid), supp_start_(supp_start), F_(F), P_(P), Q_(Q), l_(l), s_(s), bus_(bus), rng_(rng), ms_(ms), tx_(tx) {
        inv_.assign(P + 1, 0); // Allocate inventory vector with P+1 items (1-based indexing) and initialize to 0
    }
    
    void initialize(double startTime) override {
        for(int i=1; i<=P_; ++i) inv_[i] = rng_.uniformInt(0, Q_);
        nextTime_ = startTime + l_ + s_;
    }

    double nextEventTime() const override {
        return nextTime_;
    }

    void handleEvent(double currentTime) override {
        auto &inbox = bus_.netToProc(pid_);
        while (!inbox.empty()) {
            Message m = inbox.front(); inbox.pop();
            bool isSupply = (m.sender >= supp_start_ && m.sender < supp_start_ + F_);
            if (isSupply) {
                inv_[m.item] += (int)m.quantity;
            } else {
                // This is a customer request forwarded by the server
                int req = (int)m.quantity;
                if (inv_[m.item] < req) {
                    ms_ += req - inv_[m.item]; // Increment missed sells by the unsatisfied quantity
                }
                int prov = std::min(req, inv_[m.item]);
                inv_[m.item] -= prov;
                
                // Track total transactions correctly (requests processed by DB)
                tx_++;
            }
        }
        nextTime_ = currentTime + l_ + s_;
    }
    
private:
    int pid_, supp_start_, F_, P_, Q_;
    double l_, s_, nextTime_;
    MessageBus &bus_;
    RandomGenerator &rng_;
    std::vector<int> inv_;
    long long &ms_;
    long long &tx_;
};
        

class CustomerProcess : public DiscreteEventProcess {
public:
    CustomerProcess(int pid, int serv_start, int S, int P, int Q, double A, double B, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), serv_start_(serv_start), S_(S), P_(P), Q_(Q), A_(A), B_(B), bus_(bus), rng_(rng) {}
    
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
        m.receiver = serv_start_ + rng_.uniformInt(0, S_ - 1);
        m.item = rng_.uniformInt(1, P_);
        m.quantity = (double)rng_.uniformInt(1, Q_);
        bus_.procToNet(pid_).push(m);
        nextTime_ = currentTime + rng_.uniform(A_, B_);
    }

private:
    int pid_, serv_start_, S_, P_, Q_;
    double A_, B_, nextTime_;
    MessageBus &bus_;
    RandomGenerator &rng_;
};

class SupplierProcess : public DiscreteEventProcess {
public:
    SupplierProcess(int pid, int database_pid, int P, int Q, double V, double W, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), database_pid_(database_pid), P_(P), Q_(Q), V_(V), W_(W), bus_(bus), rng_(rng) {}

    void initialize(double startTime) override {
        nextTime_ = startTime + rng_.uniform(V_, W_);
    }
    
    double nextEventTime() const override {
        return nextTime_;
    }

    void handleEvent(double currentTime) override {
        Message m;
        m.time = currentTime;
        m.sender = pid_;
        m.receiver = database_pid_;
        m.item = rng_.uniformInt(1, P_);
        m.quantity = (double)rng_.uniformInt(1, Q_);
        bus_.procToNet(pid_).push(m);

        nextTime_ = currentTime + rng_.uniform(V_, W_);
    }
private:
    int pid_, database_pid_, P_, Q_;
    double V_, W_, nextTime_;
    MessageBus &bus_; RandomGenerator &rng_;
};

class ServerProcess : public DiscreteEventProcess {
public:
    ServerProcess(int pid, int database_pid, double z, double v, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), database_pid_(database_pid), z_(z), v_(v), bus_(bus), rng_(rng) {}

    void initialize(double startTime) override {
        nextTime_ = startTime + z_ + v_;
    }

    double nextEventTime() const override {
        return nextTime_;
    }
    
    void handleEvent(double currentTime) override {
        auto &inbox = bus_.netToProc(pid_);

        while (!inbox.empty()) {
            Message m = inbox.front(); inbox.pop();
            if (m.sender != database_pid_) {
                // This is a customer request, forward it to the database
                Message forward;
                forward.time = currentTime;
                forward.sender = pid_;
                forward.receiver = database_pid_;
                forward.item = m.item;
                forward.quantity = m.quantity;
                bus_.procToNet(pid_).push(forward);
            }
        }
        nextTime_ = currentTime + z_ + v_;
    }
private:
    int pid_, database_pid_;
    double z_, v_;
    MessageBus &bus_; RandomGenerator &rng_;
    double nextTime_;
};

}

int main() {
    SELib::RandomGenerator rng(std::random_device{}());
    SELib::ParameterParser parser;
    if (!parser.parseFile("parameters.txt")) return 1;
    
    double H = parser.getDouble("H"); 
    int M = parser.getInt("M"); 

    int C = parser.getInt("C");
    int S = parser.getInt("S");
    int F = parser.getInt("F");
    double A = parser.getDouble("A");
    double B = parser.getDouble("B");
    double V = parser.getDouble("V");
    double W = parser.getDouble("W");
    int P = parser.getInt("P");
    int Q = parser.getInt("Q");
    double r = parser.getDouble("r");
    double w = parser.getDouble("w");
    double l = parser.getDouble("l");
    double s = parser.getDouble("s");
    double z = parser.getDouble("z");
    double v = parser.getDouble("v");

    SELib::Statistics stats;
    for (int m = 0; m < M; ++m) {
        
        // Fix: Use exact number of processes to avoid massive NetworkRouter loop delay
        int num_processes = C + F + S + 1;
        SELib::MessageBus bus(num_processes);

        SELib::DiscreteEventSystem system;
        long long missed_sells = 0;
        long long total_transactions = 0;
        
        // Define clean sequential PIDs to match the bus size bounds perfectly
        int pid_counter = 0;
        int cust_start = pid_counter; pid_counter += C;
        int supp_start = pid_counter; pid_counter += F;
        int serv_start = pid_counter; pid_counter += S;
        int database_pid = pid_counter; pid_counter += 1;

        for(int i=0; i<C; ++i) {
            system.emplaceProcess<SELib::CustomerProcess>(cust_start + i, serv_start, S, P, Q, A, B, bus, rng);
        }
        for(int i=0; i<F; ++i) {
            system.emplaceProcess<SELib::SupplierProcess>(supp_start + i, database_pid, P, Q, V, W, bus, rng);
        }
        for(int i=0; i<S; ++i) {
            system.emplaceProcess<SELib::ServerProcess>(serv_start + i, database_pid, z, v, bus, rng);
        }
        
        system.emplaceProcess<SELib::DatabaseProcess>(database_pid, supp_start, F, P, Q, l, s, bus, rng, missed_sells, total_transactions);
        
        // A minimal scan period avoids network buffering delays without dropping messages
        system.emplaceProcess<SELib::NetworkRouter>(bus, rng, 0.01);

        // Init DES simulator 
        SELib::DiscreteEventSimulator sim(system);

        // Run simulation
        sim.run(H);
        
        double sim_rate = (total_transactions > 0) ? ((double)missed_sells / total_transactions) : 0.0;
        stats.addSample(sim_rate);
    }
    
    std::ofstream outFile("results.txt");
    outFile << "2026-06-02-Alessandro-Tang-2106357" << std::endl;
    outFile << "R " << std::fixed << std::setprecision(6) << stats.mean() << std::endl;
    return 0;
}