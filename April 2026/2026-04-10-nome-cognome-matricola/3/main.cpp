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

// Can return even numbers in [2,N]
static int pickEven(int N, RandomGenerator &rng) {
    return 2 * rng.uniformInt(1, N / 2); // e.g. N=10 -> 2*(1..5) = {2,4,6,8,10}
}

// Can return odd numbers in [1,N]
static int pickOdd(int N, RandomGenerator &rng) {
    return 2 * rng.uniformInt(0, (N + 1) / 2 - 1) + 1; // e.g. N=10 -> 2*(0..4)+1 = {1,3,5,7,9}
}

class CustomerProcess : public DiscreteEventProcess {
public:
    CustomerProcess(int pid, int S, int P, int Q, double A, double B, MessageBus &bus, RandomGenerator &rng, double S0, double P0, double Q0)
        : pid_(pid), S_(S), P_(P), Q_(Q), A_(A), B_(B), bus_(bus), rng_(rng), S0_(S0), P0_(P0), Q0_(Q0) {}
    
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
        m.receiver = 100 + (rng_.bernoulli(S0_) ? pickEven(S_, rng_) : pickOdd(S_, rng_));
        m.item = rng_.bernoulli(P0_) ? pickEven(P_, rng_) : pickOdd(P_, rng_);
        m.quantity = rng_.bernoulli(Q0_) ? pickEven(Q_, rng_) : pickOdd(Q_, rng_);
        bus_.procToNet(pid_).push(m);
        nextTime_ = currentTime + rng_.uniform(A_, B_);
    }

private:
    int pid_, S_, P_, Q_;
    double A_, B_, nextTime_;
    MessageBus &bus_;
    RandomGenerator &rng_;
    double S0_, P0_, Q0_;
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
    ServerProcess(int pid, int C, int databasePid, int P, int Q, MessageBus &bus, RandomGenerator &rng, double ST0, double ST1, double SEP, double SOP, double QEA, double QOA, double T, std::vector<int> &costs, std::vector<double> &rr, double serverReadDelay = 0.0, double serverWriteDelay = 0.0)
        : pid_(pid), C_(C), databasePid_(databasePid), P_(P), Q_(Q), bus_(bus), rng_(rng), ST0_(ST0), ST1_(ST1), SEP_(SEP), SOP_(SOP), QEA_(QEA), QOA_(QOA), T_(T), costs_(costs), rr_(rr), serverReadDelay_(serverReadDelay), serverWriteDelay_(serverWriteDelay), sellBuy_(0.0), y_(0.0), updateYTime_(0.0) {
    }
    
    void initialize(double startTime) override {
        if (serverReadDelay_ > 0.0) {
            nextReadTime_ = startTime + serverReadDelay_;
        } else {
            nextReadTime_ = startTime + 0.1;
        }
        inPending_.clear();
        updateCacheTime_ = startTime + (pid_ % 2 == 0 ? ST0_ : ST1_);
        cache_.assign(P_ + 1, {0, 0});
        for (int i = 1; i <= P_; ++i) {
            cache_[i].second = costs_[i];
        }

    }

    double nextEventTime() const override {
        double t = nextReadTime_;
        t = std::min(t, updateCacheTime_);
        t = std::min(t, updateYTime_);
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

        if (currentTime >= updateCacheTime_) {

            Message m;
            m.time = currentTime;
            m.sender = pid_;
            m.receiver = databasePid_;
            m.item = pid_ % 2 == 0 ?
                    (rng_.bernoulli(SEP_) ? pickEven(P_, rng_) : pickOdd(P_, rng_)) : 
                    (rng_.bernoulli(SOP_) ? pickOdd(P_, rng_) : pickEven(P_, rng_));
            m.quantity = pid_ % 2 == 0 ?
                    (rng_.bernoulli(QEA_) ? pickEven(Q_, rng_) : pickOdd(Q_, rng_)) : 
                    (rng_.bernoulli(QOA_) ? pickOdd(Q_, rng_) : pickEven(Q_, rng_));
            bus_.procToNet(pid_).push(m);
            updateCacheTime_ = currentTime + (pid_ % 2 == 0 ? ST0_ : ST1_);
        }

        if (currentTime >= nextReadTime_) {
            bool processedMessage = false;
            auto &inbox = bus_.netToProc(pid_);
            while (!inbox.empty()) {
                processedMessage = true;
                Message m = inbox.front(); inbox.pop();
                if (m.sender == databasePid_) {
                    int received = (int)m.quantity;
                    cache_[m.item].first += received;
                    sellBuy_ -= received * cache_[m.item].second;
                } else {
                    int requested = (int)m.quantity;
                    int canSend = std::min(cache_[m.item].first, requested);
                    cache_[m.item].first -= canSend;
                    sellBuy_ += canSend * cache_[m.item].second;
                }
            }
            if (serverReadDelay_ > 0.0 && processedMessage) {
                nextReadTime_ = currentTime + serverReadDelay_;
            } else {
                nextReadTime_ = currentTime + 0.1;
            }
        }

        if (currentTime >= updateYTime_) {

            double sum = 0;
            for (int i = 1; i <= P_; ++i) {
                sum += cache_[i].first * cache_[i].second;
            }
            y_ = y_ * + T_ * 0.1 * sum;
            rr_[(pid_ -1 ) - 100] = (sellBuy_ - y_)/updateYTime_;
            updateYTime_ = currentTime + T_;
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

    double ST0_, ST1_;
    double SEP_, SOP_;
    double QEA_, QOA_;
    double T_;
    std::vector<int> &costs_;
    std::vector<double> &rr_;

    double serverReadDelay_, serverWriteDelay_;

    
    double sellBuy_;
    double y_;
    double updateYTime_;
    std::vector<std::pair<int,int>> cache_;
    double updateCacheTime_;
    
    

};

class DatabaseProcess : public DiscreteEventProcess {
public:
    DatabaseProcess(int pid, int C, int F, int S, int P, int Q, std::vector<std::pair<int, int>> &inventory, MessageBus &bus, RandomGenerator &rng, double dbReadDelay = 0.0, double dbWriteDelay = 0.0)
        : pid_(pid), C_(C), F_(F), S_(S), P_(P), Q_(Q), inventory_(inventory), bus_(bus), rng_(rng), dbReadDelay_(dbReadDelay), dbWriteDelay_(dbWriteDelay) {
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
            while (!inbox.empty()) {
                processedMessage = true;
                Message m = inbox.front(); inbox.pop();
                bool isSupply = (m.sender >= C_ && m.sender < C_ + F_);
                if (isSupply) {
                    inventory_[m.item].first += (int)m.quantity;
                } else {
                    int requested = (int)m.quantity;
                    int canSend = std::min(inventory_[m.item].first, requested);
                    inventory_[m.item].first -= canSend;
                    Message reply;
                    reply.time = currentTime;
                    reply.sender = pid_;
                    reply.receiver = m.sender;
                    reply.item = m.item;
                    reply.quantity = (double)canSend;
                    bus_.procToNet(pid_).push(reply);
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
    std::vector<std::pair<int, int>> &inventory_;
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
    double T = parser.getDouble("T");

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

        SELib::MessageBus bus(200);
        SELib::DiscreteEventSystem system;
        int databasePid = 199;

        std::vector<int> costs(P + 1, 0);
        std::vector<std::pair<int, int>> inventory;
        inventory.assign(P + 1, {0, 0}); // 1-based indexing for items
        for (int i = 1; i <= P; ++i) {
            inventory[i].first = rng.uniformInt(0, Q);
            inventory[i].second = rng.uniformInt(1, 100);
            costs[i] = inventory[i].second;
        }
        std::vector<double> rr(S, 0);
        
        for(int i=0; i<C; ++i) {
            system.emplaceProcess<SELib::CustomerProcess>(i, S, P, Q, A, B, bus, rng, S0, P0, Q0);
        }
        for(int i=0; i<F; ++i) {
            system.emplaceProcess<SELib::SupplierProcess>(C+i, databasePid, P, Q, V, W, bus, rng);
        }
        for(int i=0; i<S; ++i) {
            system.emplaceProcess<SELib::ServerProcess>(100+(i+1), C, databasePid, P, Q, bus, rng, ST0, ST1, SEP, SOP, QEA, QOA, T, costs, rr, 0, 0); // (serverReadDelay, serverWriteDelay)
        }
        system.emplaceProcess<SELib::DatabaseProcess>(databasePid, C, F, S, P, Q, inventory, bus, rng, 0, 0); // (dbReadDelay, dbWriteDelay)
        system.emplaceProcess<SELib::NetworkRouter>(bus, rng, 0.0001, r); // (scanPeriod, networkDelay)

        SELib::DiscreteEventSimulator sim(system);
        sim.run(H);

        double result = 0;
        for (int i = 0; i < S; ++i) {
            result += rr[i];
        }

        stats.addSample(result / H);
    }
    std::ofstream outFile("results.txt");
    outFile << "2026-06-02-Alessandro-Tang-2106357" << std::endl;
    outFile << "RR " << std::fixed << std::setprecision(6) << stats.mean() << std::endl;
    return 0;
}


/*
    Graveyard of old code pieces versions

    Siccome si usa WHILE allora non esiste un db read delay
    quindi sarà sempre nextReadTime_ = currentTime + 0.1;
    Pezzo di codice da rimuovere se si ha un db read delay
    e quindi si usa IF al posto di WHILE per processare un solo messaggio alla volta
    in quanto se non c'è un messaggio da leggere allora andrà qui e siccome c'è
    un db read delay allora farà nextReadTime_ = currentTime + dbReadDelay_
    anche se non ha letto un messaggio per poter giustificare un aumento di db read delay
    
    if (dbReadDelay_ > 0.0) {
        nextReadTime_ = currentTime + dbReadDelay_;
    } else {
        nextReadTime_ = currentTime + 0.1;
    }
    
    In tal caso aggiungere il codice seguente al posto di quello sopra
    
    else { nextReadTime_ = currentTime + 0.1; }

    -----------------------------------------

    Alternative inventory initialization that includes cost values

    std::vector<std::pair<int, int>> inventory;
    inventory.assign(P + 1, {0, 0}); // (quantity, cost)
    for (int i = 1; i <= P; ++i) {
        inventory[i].first = rng.uniformInt(0, Q);
        inventory[i].second = rng.uniformInt(1, maxCost); 
    }


*/