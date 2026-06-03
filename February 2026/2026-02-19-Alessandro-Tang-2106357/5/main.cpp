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
        // Random time + fixed write delay + fixed read delay
        // Write delay is for the message being sent to the server, which is explicitly modeled.
        // Read delay is for the implied reply from the server
        // which is not explicitly modeled but affects the inter-arrival time of customer requests.
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
    SupplierProcess(int pid, int database_pid, int P, int Q, double V, double W, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), database_pid_(database_pid), P_(P), Q_(Q), V_(V), W_(W), bus_(bus), rng_(rng) {}

    void initialize(double startTime) override {
        // Random time + fixed write delay
        // Write delay is for the message being sent to the server, which is explicitly modeled.
        // There is no read delay for suppliers since they do not wait for a reply from the DB
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
    MessageBus &bus_;
    RandomGenerator &rng_;
};

class ServerProcess : public DiscreteEventProcess {
public:
    ServerProcess(int pid, int C, int database_pid, int P, int Q, double r, double w, double z, double v, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), C_(C), database_pid_(database_pid), P_(P), Q_(Q), r_(r), w_(w), z_(z), v_(v), bus_(bus), rng_(rng) {}

    void initialize(double startTime) override {
        nextTime_ = startTime + r_ + z_;
    }

    double nextEventTime() const override {
        return nextTime_;
    }
    
    void handleEvent(double currentTime) override {
        
        // Check the first message in the inbox
        auto &inbox = bus_.netToProc(pid_);

        if (!inbox.empty()) {
            Message m = inbox.front(); inbox.pop();
            if (m.sender == database_pid_) {
                // Nothing
            } else {
                // This is a customer request, forward it to the database
                Message forward;
                forward.time = currentTime;
                forward.sender = pid_;
                forward.receiver = database_pid_;
                forward.item = m.item;
                forward.quantity = m.quantity;
                bus_.procToNet(pid_).push(forward);
            }
            nextTime_ = currentTime + 
                        w_ + v_ + // Redirect customer request to DB
                        r_ + z_ + // Process DB availability and inventory check
                        w_ + v_ + // Send to DB that he will send items to customer so DB needs to update inventory
                        w_ + v_ + // Send reply to customer that his request is being processed
                        r_ + z_;  // Delay of the next request being processed by the server after this one

        } else {
            nextTime_ = currentTime + r_ + z_;
        }
        
        
    }
private:
    int pid_, C_, database_pid_, P_, Q_;
    double r_, w_, z_, v_;
    MessageBus &bus_;
    RandomGenerator &rng_;
    double nextTime_;
};

class DatabaseProcess : public DiscreteEventProcess {
public:
    DatabaseProcess(int pid, int C,int S, int F, int P, int Q, double r, double w, double l, double s, MessageBus &bus, RandomGenerator &rng, long long &ms, long long &transactions)
        : pid_(pid), C_(C), S_(S), F_(F), P_(P), Q_(Q), r_(r), w_(w), l_(l), s_(s), bus_(bus), rng_(rng), ms_(ms), transactions_(transactions) {

        inv_.assign(P + 1, 0); // Allocate inventory vector with P+1 items (1-based indexing) and initialize to 0
    }
    void initialize(double startTime) override {
        for(int i=1; i<=P_; ++i) inv_[i] = rng_.uniformInt(0, Q_);
        nextTime_ = startTime +r_ + l_;
    }

    double nextEventTime() const override {
        return nextTime_;
    }

    void handleEvent(double currentTime) override {
        auto &inbox = bus_.netToProc(pid_);
        if (!inbox.empty()) {

            Message m = inbox.front(); inbox.pop();
            bool isSupply = (m.sender >= C_ && m.sender < C_ + F_);
            if (isSupply) {
                inv_[m.item] += (int)m.quantity;
                nextTime_ = currentTime + r_ + l_;
            } else {
                // This is a customer request forwarded by the server
                int req = (int)m.quantity;
                if (inv_[m.item] < req) {
                    ms_ += req - inv_[m.item]; // Increment missed sells by the unsatisfied quantity
                }
                int prov = std::min(req, inv_[m.item]);
                inv_[m.item] -= prov;
                transactions_++;
                nextTime_ = currentTime +
                            r_ + l_ + // Read redirected customer request from server
                            w_ + s_ + // Send to server inventory and availability info
                            r_ + l_ + // Read reply from server that he will send items to customer so DB needs to update inventory
                            r_ + l_; // Delay of the next request being processed by the DB after this one
            }
        } else {
            nextTime_ = currentTime + r_ + l_;
        }
        
    }
    
private:
    int pid_, C_, S_, F_, P_, Q_;
    double r_, w_, l_, s_, nextTime_;
    MessageBus &bus_;
    RandomGenerator &rng_;
    std::vector<int> inv_;
    long long &ms_;
    long long &transactions_;
};

double runMsims(double H, int M, int C, int S, int F, double A, double B, double V, double W, int P, int Q, double r, double w, double l, double s, double z, double v, SELib::RandomGenerator &rng) {
    SELib::Statistics stats;
    for (int m = 0; m < M; ++m) {

        // Init message bus
        SELib::MessageBus bus(200);

        // Init DES system
        SELib::DiscreteEventSystem system;
        long long missed_sells = 0;
        long long transactions = 0;
        int database_pid = 199; // Fixed PID for the centralized database process

        // Create processes: C customers, F suppliers, S servers, 1 router and 1 centralized database
        for(int i=0; i<C; ++i) {
            system.emplaceProcess<SELib::CustomerProcess>(i, S, P, Q, A, B, bus, rng);
        }
        for(int i=0; i<F; ++i) {
            system.emplaceProcess<SELib::SupplierProcess>(C+i, database_pid, P, Q, V, W, bus, rng);
        }
        for(int i=0; i<S; ++i) {
            system.emplaceProcess<SELib::ServerProcess>(100+i, C, database_pid, P, Q, r, w, z, v, bus, rng);
        }
        system.emplaceProcess<SELib::DatabaseProcess>(database_pid, C, S, F, P, Q, r, w, l, s, bus, rng, missed_sells, transactions);
        // If 0.01 then 200 process -> 200 * 0.01 = 2s of delay just for the router to do one scan of all processes,
        // which is too much. With 0.0001 it's 0.02s which is acceptable for our simulation scale
        // Warn: if set to very low values like 0.00001, the router might cause a bottleneck and
        // significantly increase simulation time due to excessive scanning frequency.
        system.emplaceProcess<SELib::NetworkRouter>(bus, rng, 0.0001); 

        // Init DES simulator 
        SELib::DiscreteEventSimulator sim(system);

        // Run simulation
        sim.run(H);

        stats.addSample((double)missed_sells / transactions);
    }
    return stats.mean();
}

}

int main() {
    SELib::RandomGenerator rng(std::random_device{}());
    SELib::ParameterParser parser;
    if (!parser.parseFile("parameters.txt")) return 1;
    
    double H = parser.getDouble("H"); // simulation horizon
    int M = parser.getInt("M"); // number of simulations

    int C = parser.getInt("C");
    int S = parser.getInt("S");
    //int F = parser.getInt("F");
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

    double a = parser.getDouble("a");
    double b = parser.getDouble("b");
    int G = parser.getInt("G");

    // New ex4 values
    int best_k_for_F = 0;
    double min_cost_J = 1e18;
    double min_missed_sells_R = 0;

    for (int k = 0; k <= G; ++k) {
        double F = 1 + k;
        double R = SELib::runMsims(H, M, C, S, F, A, B, V, W, P, Q, r, w, l, s, z, v, rng);
        double J = a*F + b*R;
        if (J < min_cost_J) {
            min_cost_J = J;
            best_k_for_F = k;
            min_missed_sells_R = R;
        }
    }
    
    std::ofstream outFile("results.txt");
    outFile << "2026-06-02-Alessandro-Tang-2106357" << std::endl;
    outFile << "R " << min_missed_sells_R << std::endl;
    outFile << "F " << 1 + best_k_for_F << std::endl;
    outFile << "J " << min_cost_J << std::endl;
    return 0;
}