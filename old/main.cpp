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

static int pickEven(int N, RandomGenerator &rng) {
    return 2 * rng.uniformInt(1, N / 2);
}
static int pickOdd(int N, RandomGenerator &rng) {
    return 2 * rng.uniformInt(0, (N + 1) / 2 - 1) + 1; // if N=10 -> 2*(0..4)+1 = {1,3,5,7,9}
}

class CustomerProcess : public DiscreteEventProcess {
public:
    CustomerProcess(int pid, int S, int P, int Q, double A, double B, double S0, double P0, double Q0, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), S_(S), P_(P), Q_(Q), A_(A), B_(B), S0_(S0), P0_(P0), Q0_(Q0), bus_(bus), rng_(rng) {}
    
    void initialize(double startTime) override {
        nextTime_ = startTime + rng_.uniform(A_, B_);
    }

    double nextEventTime() const override {
        return nextTime_;
    }

    void handleEvent(double currentTime) override {

        int server = rng_.bernoulli(S0_) ? pickEven(S_, rng_) : pickOdd(S_, rng_);
        int item = rng_.bernoulli(P0_) ? pickEven(P_, rng_) : pickOdd(P_, rng_);
        int quantity = rng_.bernoulli(Q0_) ? pickEven(Q_, rng_) : pickOdd(Q_, rng_);
        
        Message m;
        m.time = currentTime;
        m.sender = pid_;
        m.receiver = 100 + (server - 1); // Server PIDs are [100, 100+S-1]
        m.item = item;
        m.quantity = quantity;
        bus_.procToNet(pid_).push(m);
        nextTime_ = currentTime + rng_.uniform(A_, B_);
    }

private:
    int pid_, S_, P_, Q_;
    double A_, B_, S0_, P0_, Q0_, nextTime_;
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
    MessageBus &bus_;
    RandomGenerator &rng_;
};

class ServerProcess : public DiscreteEventProcess {
public:
    ServerProcess(int pid, int C, int database_pid, int P, int Q, double ST0, double ST1, double SEP, double SOP, double QEA, double QOA, std::vector<int> costs, MessageBus &bus, RandomGenerator &rng, double serverReadDelay = 0.0, double serverWriteDelay = 0.0)
        : pid_(pid), C_(C), database_pid_(database_pid), P_(P), Q_(Q), ST0_(ST0), ST1_(ST1), SEP_(SEP), SOP_(SOP), QEA_(QEA), QOA_(QOA), costs_(costs), bus_(bus), rng_(rng), serverReadDelay_(serverReadDelay), serverWriteDelay_(serverWriteDelay), sell_buy_(0.0), y_(0.0) {
        cache_.assign(P_ + 1, {0, 0}); // Initialize cache with P+1 items (1-based indexing) and (quantity, cost) pairs set to (0,0)
    }
    
    void initialize(double startTime) override {
        nextReadTime_ = startTime + 0.1;
        inPending_.clear();

        updateCacheTime_ = startTime + (pid_ % 2 == 0 ? ST0_ : ST1_);
        sell_buy_ = 0.0;
        y_ = 0.0;
        // Initialize cost for each item in the cache
        for (int i = 1; i <= P_; ++i) {
            cache_[i].second = costs_[i]; 
        }
    }

    double nextEventTime() const override {
        double t = nextReadTime_;
        t = std::min(t, updateCacheTime_);
        for (const auto &m : inPending_) {
            if (m.deliveryTime < t) {
                t = m.deliveryTime;
            }
        }
        return t;
    }
    
    void handleEvent(double currentTime) override {

        pendingReady(currentTime);
        
        if (currentTime >= updateCacheTime_) {

            // ...

            updateCacheTime_ = currentTime + (pid_ % 2 == 0 ? ST0_ : ST1_);
        }

        if (currentTime >= nextReadTime_) {
            auto &inbox = bus_.netToProc(pid_);
            while (!inbox.empty()) {
                Message m = inbox.front(); inbox.pop();
                if (m.sender == database_pid_) {
                    Message reply;
                    reply.time = currentTime;
                    reply.sender = pid_;
                    reply.receiver = m.sender;
                    reply.item = m.item;
                    reply.quantity = m.quantity;
                    if (serverWriteDelay_ <= 0.0) {
                        bus_.procToNet(reply.receiver).push(reply);
                    } else {
                        inPending_.push_back({reply, currentTime + serverWriteDelay_});
                    }
                } else {
                    // Customer message
                }
            }
            /** Alternative to while-loop if there is a server read delay:
            if (!inbox.empty()) {
                Message m = inbox.front(); inbox.pop();
                ...
                nextReadTime_ = currentTime + serverReadDelay_;
            } else { 
                nextReadTime_ = currentTime + 0.1;
            }
            */

            // Generalized nextReadTime_ update to handle both cases (with or without server read delay)
            if (serverReadDelay_ > 0.0) {
                nextReadTime_ = currentTime + serverReadDelay_;
            } else {
                nextReadTime_ = currentTime + 0.1;
            }
        }
    }

private:

    struct PendingOutMessage {
        Message msg;
        double deliveryTime; // currentTime + server write delay
    };

    void pendingReady(double currentTime) {
        auto ip = inPending_.begin();
        while (ip != inPending_.end()) {
            if (ip->deliveryTime <= currentTime) {
                bus_.procToNet(ip->msg.sender).push(ip->msg);
                ip = inPending_.erase(ip);
            } else {
                ++ip;
            }
        }
    }

    int pid_, C_, database_pid_, P_, Q_;
    
    double ST0_, ST1_, SEP_, SOP_, QEA_, QOA_;
    std::vector<int> costs_;

    MessageBus &bus_;
    RandomGenerator &rng_;
    double nextReadTime_;
    std::vector<PendingOutMessage> inPending_;
    double serverReadDelay_, serverWriteDelay_;

    std::vector<std::pair<int, int>> cache_; // (quantity, cost)
    double sell_buy_;
    double y_;
    double updateCacheTime_;
    
};

class DatabaseProcess : public DiscreteEventProcess {
public:
    DatabaseProcess(int pid, int C,int S, int F, int P, int Q, double r, std::vector<std::pair<int, int>> &inv, MessageBus &bus, RandomGenerator &rng, double dbReadDelay = 0.0, double dbWriteDelay = 0.0)
        : pid_(pid), C_(C), S_(S), F_(F), P_(P), Q_(Q), r_(r), inv_(inv), bus_(bus), rng_(rng), dbReadDelay_(dbReadDelay), dbWriteDelay_(dbWriteDelay) {
    }
    void initialize(double startTime) override {
        nextReadTime_ = startTime + 0.1;
        inPending_.clear();
    }

    double nextEventTime() const override {
        double t = nextReadTime_;
        for (const auto &m : inPending_) {
            if (m.deliveryTime < t) {
                t = m.deliveryTime;
            }
        }
        return t;
    }

    void handleEvent(double currentTime) override {

        pendingReady(currentTime);

        if (currentTime >= nextReadTime_) {
            auto &inbox = bus_.netToProc(pid_);
            while (!inbox.empty()) {
                Message m = inbox.front(); inbox.pop();
                bool isSupply = (m.sender >= C_ && m.sender < C_ + F_);
                if (isSupply) {
                    inv_[m.item].first += (int)m.quantity;
                    if (dbReadDelay_ > 0.0) {
                        nextReadTime_ = currentTime + dbReadDelay_;
                    } else {
                        nextReadTime_ = currentTime + 0.1;
                    }
                } else {
                    // Server messages
                }
            }
            /** Alternative to while-loop if there is a db read delay:
            if (!inbox.empty()) {
                Message m = inbox.front(); inbox.pop();
                ...
                nextReadTime_ = currentTime + dbReadDelay_;
            } else { 
                nextReadTime_ = currentTime + 0.1;
            }
            */
        }
    }
    
private:

    struct PendingOutMessage {
        Message msg;
        double deliveryTime; // currentTime + db write delay
    };

    void pendingReady(double currentTime) {
        auto ip = inPending_.begin();
        while (ip != inPending_.end()) {
            if (ip->deliveryTime <= currentTime) {
                bus_.procToNet(ip->msg.sender).push(ip->msg);
                ip = inPending_.erase(ip);
            } else {
                ++ip;
            }
        }
    }
    int pid_, C_, S_, F_, P_, Q_;

    double r_;
    std::vector<std::pair<int, int>> &inv_;

    MessageBus &bus_;
    RandomGenerator &rng_;
    double nextReadTime_;
    std::vector<PendingOutMessage> inPending_;
    double dbReadDelay_, dbWriteDelay_;
    
    
};

}

int main() {
    SELib::RandomGenerator rng(std::random_device{}());
    SELib::ParameterParser parser;
    if (!parser.parseFile("parameters.txt")) return 1;
    
    double H = parser.getDouble("H"); // simulation horizon
    int M = parser.getInt("M"); // number of simulations

    int C = parser.getInt("C");
    int S = parser.getInt("S");
    int F = parser.getInt("F");

    double A = parser.getDouble("A");
    double B = parser.getDouble("B");

    double V = parser.getDouble("V");
    double W = parser.getDouble("W");

    int P = parser.getInt("P");
    int Q = parser.getInt("Q");

    double r = parser.getDouble("r"); // Uniform delay
    
    double S0 = parser.getDouble("S0");
    double P0 = parser.getDouble("P0");
    double Q0 = parser.getDouble("Q0");

    double ST0 = parser.getDouble("ST0");
    double ST1 = parser.getDouble("ST1");

    double SEP = parser.getDouble("SEP");
    double SOP = parser.getDouble("SOP");
    double QEA = parser.getDouble("QEA");
    double QOA = parser.getDouble("QOA");

    SELib::Statistics stats;
    for (int m = 0; m < M; ++m) {

        // Init message bus
        SELib::MessageBus bus(200);

        // Init DES system
        SELib::DiscreteEventSystem system;

        double reward_rate = 0.0;

        std::vector<std::pair<int, int>> inv;
        std::vector<int> costs(P + 1, 0); // For server
        inv.assign(P + 1, {0, 0}); // (quantity, cost)
        for (int i = 1; i <= P; ++i) {
            inv[i].first = rng.uniformInt(0, Q);
            inv[i].second = rng.uniformInt(1, 100);
            costs[i] = inv[i].second;
        }

        int database_pid = 199; // Fixed PID for the centralized database process

        // Create processes: C customers, F suppliers, S servers, 1 router and 1 centralized database
        for(int i=0; i<C; ++i) {
            system.emplaceProcess<SELib::CustomerProcess>(i, S, P, Q, A, B, S0, P0, Q0, bus, rng);
        }
        for(int i=0; i<F; ++i) {
            system.emplaceProcess<SELib::SupplierProcess>(C+i, database_pid, P, Q, V, W, bus, rng);
        }
        for(int i=0; i<S; ++i) {
            system.emplaceProcess<SELib::ServerProcess>(100+i, C, database_pid, P, Q, ST0, ST1, SEP, SOP, QEA, QOA, costs, bus, rng);
        }
        system.emplaceProcess<SELib::DatabaseProcess>(database_pid, C, S, F, P, Q, r, inv, bus, rng);
        // If 0.01 then 200 process -> 200 * 0.01 = 2s of delay just for the router to do one scan of all processes,
        // which is too much. With 0.0001 it's 0.02s which is acceptable for our simulation scale
        // Warn: if set to very low values like 0.00001, the router might cause a bottleneck and
        // significantly increase simulation time due to excessive scanning frequency.
        system.emplaceProcess<SELib::NetworkRouter>(bus, rng, 0.0001, r); 

        // Init DES simulator 
        SELib::DiscreteEventSimulator sim(system);

        // Run simulation
        sim.run(H);

        stats.addSample(reward_rate / H);
    }
    std::ofstream outFile("results.txt");
    outFile << "2026-06-02-Alessandro-Tang-2106357" << std::endl;
    outFile << "R " << std::fixed << std::setprecision(6) << stats.mean() << std::endl;
    return 0;
}