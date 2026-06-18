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
    return 2 * rng.uniformInt(1, N / 2); // e.g. N=10 -> 2*(1..5) = {2,4,6,8,10}
}
static int pickOdd(int N, RandomGenerator &rng) {
    return 2 * rng.uniformInt(0, (N + 1) / 2 - 1) + 1; // e.g. N=10 -> 2*(0..4)+1 = {1,3,5,7,9}
}

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
    SupplierProcess(int pid, int databasePid, int P, int Q, double V, double W, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), databasePid_(databasePid), P_(P), Q_(Q), V_(V), W_(W), bus_(bus), rng_(rng) {}

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
        m.receiver = databasePid_;
        m.item = rng_.uniformInt(1, P_);
        m.quantity = (double)rng_.uniformInt(1, Q_);
        bus_.procToNet(pid_).push(m);
        nextTime_ = currentTime + rng_.uniform(V_, W_);
    }
private:
    int pid_, databasePid_, P_, Q_;
    double V_, W_, nextTime_;
    MessageBus &bus_;
    RandomGenerator &rng_;
};

class ServerProcess : public DiscreteEventProcess {
public:
    ServerProcess(int pid, int C, int databasePid, int P, int Q, MessageBus &bus, RandomGenerator &rng, double serverReadDelay = 0.0, double serverWriteDelay = 0.0)
        : pid_(pid), C_(C), databasePid_(databasePid), P_(P), Q_(Q), bus_(bus), rng_(rng), serverReadDelay_(serverReadDelay), serverWriteDelay_(serverWriteDelay) {
    }
    
    void initialize(double startTime) override {
        if (serverReadDelay_ > 0.0) {
            nextReadTime_ = startTime + serverReadDelay_;
        } else {
            nextReadTime_ = startTime + 0.1;
        }
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
        if (serverWriteDelay_ > 0.0) {
            pendingReady(currentTime);
        }
        if (currentTime >= nextReadTime_) {
            bool processedMessage = false;
            auto &inbox = bus_.netToProc(pid_);
            if (!inbox.empty()) {
                processedMessage = true;
                Message m = inbox.front(); inbox.pop();
                if (m.sender == databasePid_) {
                    // Ignore the message from database
                } else {
                    Message forward;
                    forward.time = currentTime;
                    forward.sender = pid_;
                    forward.receiver = databasePid_;
                    forward.item = m.item;
                    forward.quantity = m.quantity;
                    if (serverWriteDelay_ <= 0.0) {
                        bus_.procToNet(pid_).push(forward);
                    } else {
                        inPending_.push_back({forward, currentTime + serverWriteDelay_});
                    }
                }
            }
            if (serverReadDelay_ > 0.0 && processedMessage) {
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

    int pid_, C_, databasePid_, P_, Q_;
    MessageBus &bus_;
    RandomGenerator &rng_;
    double nextReadTime_;
    std::vector<PendingOutMessage> inPending_;
    double serverReadDelay_, serverWriteDelay_;
};

class DatabaseProcess : public DiscreteEventProcess {
public:
    DatabaseProcess(int pid, int C, int F, int S, int P, int Q, std::vector<int> &inventory, MessageBus &bus, RandomGenerator &rng, double &missedSales, double &transactions, double dbReadDelay = 0.0, double dbWriteDelay = 0.0)
        : pid_(pid), C_(C), F_(F), S_(S), P_(P), Q_(Q), inventory_(inventory), bus_(bus), rng_(rng), missedSales_(missedSales), transactions_(transactions), dbReadDelay_(dbReadDelay), dbWriteDelay_(dbWriteDelay) {
    }
    void initialize(double startTime) override {
        if (dbReadDelay_ > 0.0) {
            nextReadTime_ = startTime + dbReadDelay_;
        } else {
            nextReadTime_ = startTime + 0.1;
        }
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
        if (dbWriteDelay_ > 0.0) {
            pendingReady(currentTime);
        }
        if (currentTime >= nextReadTime_) {
            bool processedMessage = false;
            auto &inbox = bus_.netToProc(pid_);
            if (!inbox.empty()) {
                processedMessage = true;
                Message m = inbox.front(); inbox.pop();
                bool isSupply = (m.sender >= C_ && m.sender < C_ + F_);
                if (isSupply) {
                    inventory_[m.item] += (int)m.quantity;
                } else {
                    int requested = (int)m.quantity;
                    if (inventory_[m.item] < requested) {
                        missedSales_ += (requested - inventory_[m.item]); // (+1 missed transition, +missed quantity)
                    }
                    int canSend = std::min(inventory_[m.item], requested);
                    inventory_[m.item] -= canSend;
                    transactions_++;
                }
            }
            if (dbReadDelay_ > 0.0 && processedMessage) {
                nextReadTime_ = currentTime + dbReadDelay_;
            } else {
                nextReadTime_ = currentTime + 0.1;
            }
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

    int pid_, C_, F_, S_, P_, Q_;
    std::vector<int> &inventory_;
    MessageBus &bus_;
    RandomGenerator &rng_;

    double &missedSales_;
    double &transactions_;

    double nextReadTime_;
    std::vector<PendingOutMessage> inPending_;
    double dbReadDelay_, dbWriteDelay_;
};

}

int main() {
    SELib::RandomGenerator rng(std::random_device{}());
    SELib::ParameterParser parser;
    if (!parser.parseFile("parameters.txt")) return 1;
    
    double H = parser.getDouble("H"); // Simulation horizon
    int M = parser.getInt("M"); // Number of simulations

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

    pickEven(S, rng); // To not have unused function warning
    pickOdd(S, rng); // To not have unused function warning

    SELib::Statistics stats;
    for (int m = 0; m < M; ++m) {

        SELib::MessageBus bus(200);
        SELib::DiscreteEventSystem system;
        int databasePid = 199;

        double missedSales = 0.0;
        double transactions = 0.0;
        std::vector<int> inventory;
        inventory.assign(P + 1, 0); // 1-based indexing for items
        for (int i = 1; i <= P; ++i) {
            inventory[i] = rng.uniformInt(0, Q);
        }
        
        for(int i=0; i<C; ++i) {
            system.emplaceProcess<SELib::CustomerProcess>(i, S, P, Q, A, B, bus, rng);
        }
        for(int i=0; i<F; ++i) {
            system.emplaceProcess<SELib::SupplierProcess>(C+i, databasePid, P, Q, V, W, bus, rng);
        }
        for(int i=0; i<S; ++i) {
            system.emplaceProcess<SELib::ServerProcess>(100+i, C, databasePid, P, Q, bus, rng, z, v); // (serverReadDelay, serverWriteDelay)
        }
        system.emplaceProcess<SELib::DatabaseProcess>(databasePid, C, F, S, P, Q, inventory, bus, rng, missedSales, transactions, l, s); // (dbReadDelay, dbWriteDelay)
        system.emplaceProcess<SELib::NetworkRouter>(bus, rng, 0.0001, r + w); // (scanPeriod, networkDelay)

        SELib::DiscreteEventSimulator sim(system);
        sim.run(H);
        stats.addSample(missedSales / transactions);
    }
    std::ofstream outFile("results.txt");
    outFile << "2026-06-02-Alessandro-Tang-2106357" << std::endl;
    outFile << "R " << std::fixed << std::setprecision(6) << stats.mean() << std::endl;
    return 0;
}