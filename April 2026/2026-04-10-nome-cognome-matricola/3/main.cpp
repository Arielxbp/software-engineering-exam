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
    CustomerProcess(int pid, int S, int P, int Q, double A, double B, double S0, double P0, double Q0, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), S_(S), P_(P), Q_(Q), A_(A), B_(B), S0_(S0), P0_(P0), Q0_(Q0), bus_(bus), rng_(rng) {}
    
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
        
        bool even_if_true;

        // Choose server
        int current_server_index;
        rng_.bernoulli(S0_) ? even_if_true = true : even_if_true = false;
        if (even_if_true) {
            // Choose only even indexed servers
            current_server_index = rng_.uniformInt(0, S_ - 1);
            if (current_server_index % 2 != 0) {
                current_server_index--; // Adjust to even index
            }
        } else {
            current_server_index = rng_.uniformInt(0, S_ - 1);
            if (current_server_index % 2 == 0) {
                current_server_index++; // Adjust to odd index
            }
        }

        // Choose item
        int current_item_index;
        rng_.bernoulli(P0_) ? even_if_true = true : even_if_true = false;
        if (even_if_true) {
            // Choose only even indexed servers
            current_item_index = rng_.uniformInt(0, P_ - 1);
            if (current_item_index % 2 != 0) {
                current_item_index--; // Adjust to even index
            }
        } else {
            current_item_index = rng_.uniformInt(0, P_ - 1);
            if (current_item_index % 2 == 0) {
                current_item_index++; // Adjust to odd index
            }
        }

        // Choose quantity
        int current_quantity;
        rng_.bernoulli(Q0_) ? even_if_true = true : even_if_true = false;
        if (even_if_true) {
            // Choose only even indexed servers
            current_quantity = rng_.uniformInt(1, Q_);
            if (current_quantity % 2 != 0) {
                current_quantity--; // Adjust to even index
            }
        } else {
            current_quantity = rng_.uniformInt(1, Q_);
            if (current_quantity % 2 == 0) {
                current_quantity++; // Adjust to odd index
            }
        }

        Message m;
        m.time = currentTime;
        m.sender = pid_;
        m.receiver = 100 + current_server_index;
        m.item = current_item_index + 1;
        m.quantity = current_quantity;
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
    ServerProcess(int pid, int C, int database_pid, int P, int Q, double ST0, double ST1, double SEP, double SOP, double QEA, double QOA, double r, std::vector<int> costs, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), C_(C), database_pid_(database_pid), P_(P), Q_(Q), ST0_(ST0), ST1_(ST1), SEP_(SEP), SOP_(SOP), QEA_(QEA), QOA_(QOA), r_(r), costs_(costs), bus_(bus), rng_(rng) {
        cache.assign(P_ + 1, {0, 0}); // Initialize cache with P+1 items (1-based indexing) and (quantity, cost) pairs set to (0,0)
        sell_buy = 0.0;
    }
    
    void initialize(double startTime) override {
        nextTime_ = startTime + r_;

        for (int i = 1; i <= P_; ++i) {
            cache[i].second = costs_[i]; // Initialize cost for each item in the cache
        }
    }

    double nextEventTime() const override {
        return nextTime_;
    }
    
    void handleEvent(double currentTime) override {
        
        // To update cache
        int item_index;
        int quantity;
        if (pid_%2 == 0) {
            if (currentTime - lastTime_ > ST0_) {
                bool even_if_true;
                rng_.bernoulli(SEP_) ? even_if_true = true : even_if_true = false;
                if (even_if_true) {
                    // Choose only even indexed items
                    item_index = rng_.uniformInt(1, P_);
                    if (item_index % 2 != 0) {
                        item_index--; // Adjust to even index
                    }
                } else {
                    item_index = rng_.uniformInt(1, P_);
                    if (item_index % 2 == 0) {
                        item_index++; // Adjust to odd index
                    }
                }
                rng_.bernoulli(QEA_) ? even_if_true = true : even_if_true = false;
                if (even_if_true) {
                    // Choose only even indexed quantities
                    quantity = rng_.uniformInt(1, Q_);
                    if (quantity % 2 != 0) {
                        quantity--; // Adjust to even index
                    }
                } else {
                    quantity = rng_.uniformInt(1, Q_);
                    if (quantity % 2 == 0) {
                        quantity++; // Adjust to odd index
                    }
                }
                Message m;
                m.time = currentTime;
                m.sender = pid_;
                m.receiver = database_pid_;
                m.item = item_index;
                m.quantity = (double)quantity;
                bus_.procToNet(pid_).push(m);

                cache[item_index].first += quantity;
                sell_buy -= quantity * cache[item_index].second;

                nextTime_ = currentTime + r_;
                lastTime_ = currentTime;
                return;
            }
        } else {
            if (currentTime - lastTime_ > ST1_) {
                bool odd_if_true;
                rng_.bernoulli(SOP_) ? odd_if_true = true : odd_if_true = false;
                if (odd_if_true) {
                    // Choose only odd indexed items
                    item_index = rng_.uniformInt(1, P_);
                    if (item_index % 2 == 0) {
                        item_index++; // Adjust to odd index
                    }
                } else {
                    item_index = rng_.uniformInt(1, P_);
                    if (item_index % 2 == 0) {
                        item_index++; // Adjust to odd index
                    }
                }
                rng_.bernoulli(QOA_) ? odd_if_true = true : odd_if_true = false;
                if (odd_if_true) {
                    // Choose only odd indexed quantities
                    quantity = rng_.uniformInt(1, Q_);
                    if (quantity % 2 == 0) {
                        quantity++; // Adjust to odd index
                    }
                } else {
                    quantity = rng_.uniformInt(1, Q_);
                    if (quantity % 2 != 0) {
                        quantity--; // Adjust to even index
                    }
                }
                Message m;
                m.time = currentTime;
                m.sender = pid_;
                m.receiver = database_pid_;
                m.item = item_index;
                m.quantity = (double)quantity;
                bus_.procToNet(pid_).push(m);

                // cache[item_index].first += quantity;
                // sell_buy -= quantity * cache[item_index].second;

                nextTime_ = currentTime + r_;
                lastTime_ = currentTime;
                return;
            }
        }

        // Check the first message in the inbox
        auto &inbox = bus_.netToProc(pid_);

        if (!inbox.empty()) {
            Message m = inbox.front(); inbox.pop();
            
            if (m.sender == database_pid_) {
                // This is a reply from the database to a previous request
                int item_index = m.item;
                cache[item_index].first += (int)m.quantity;
                sell_buy -= (int)m.quantity * cache[item_index].second;

                if (nextTime_ < currentTime + 0.1) {
                    nextTime_ = currentTime + 0.1;
                }
                lastTime_ = currentTime;
            } else {
                // This is a new request from a customer
                int item_index = m.item;

                int decrement = std::min(cache[item_index].first, (int)m.quantity);
                
                cache[item_index].first -= decrement;

                sell_buy += decrement * cache[item_index].second;
                
                if (nextTime_ < currentTime + 0.1) {
                    nextTime_ = currentTime + 0.1;
                }
                lastTime_ = currentTime;
        }
    }
}

private:
    int pid_, C_, database_pid_, P_, Q_;
    double ST0_, ST1_, SEP_, SOP_, QEA_, QOA_;
    double r_;
    double sell_buy;
    std::vector<std::pair<int, int>> cache; // (quantity, cost)
    std::vector<int> costs_;
    MessageBus &bus_;
    RandomGenerator &rng_;
    double nextTime_;
    double lastTime_;
};

class DatabaseProcess : public DiscreteEventProcess {
public:
    DatabaseProcess(int pid, int C,int S, int F, int P, int Q, double r, std::vector<std::pair<int, int>> &inv, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), C_(C), S_(S), F_(F), P_(P), Q_(Q), r_(r), inv_(inv), bus_(bus), rng_(rng) {
    }
    void initialize(double startTime) override {
        nextTime_ = startTime + r_;
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
                inv_[m.item].first += (int)m.quantity;
                nextTime_ = currentTime + r_;
            } else {
                // Server message
                int req = (int)m.quantity;
                int available = std::min(req, inv_[m.item].first);
                inv_[m.item].first -= available;

                // Reply to server
                Message reply;
                reply.time = currentTime;
                reply.sender = pid_;
                reply.receiver = m.sender;
                reply.item = m.item;
                reply.quantity = (double)available;
                bus_.procToNet(pid_).push(reply);
                nextTime_ = currentTime + r_;
            }
        }
    }
    
private:
    int pid_, C_, S_, F_, P_, Q_;
    double r_, nextTime_;
    std::vector<std::pair<int, int>> &inv_;
    MessageBus &bus_;
    RandomGenerator &rng_;
    
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
            system.emplaceProcess<SELib::ServerProcess>(100+i, C, database_pid, P, Q, ST0, ST1, SEP, SOP, QEA, QOA, r, costs, bus, rng);
        }
        system.emplaceProcess<SELib::DatabaseProcess>(database_pid, C, S, F, P, Q, r, inv, bus, rng);
        // If 0.01 then 200 process -> 200 * 0.01 = 2s of delay just for the router to do one scan of all processes,
        // which is too much. With 0.0001 it's 0.02s which is acceptable for our simulation scale
        // Warn: if set to very low values like 0.00001, the router might cause a bottleneck and
        // significantly increase simulation time due to excessive scanning frequency.
        system.emplaceProcess<SELib::NetworkRouter>(bus, rng, 0.0001); 

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