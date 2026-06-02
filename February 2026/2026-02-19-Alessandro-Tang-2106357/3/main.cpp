#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <deque>
#include "include/DES.hpp"
#include "include/IO.hpp"
#include "include/Random.hpp"
#include "include/Stat.hpp"

namespace SELib {

class CustomerProcess : public DiscreteEventProcess {
public:
    CustomerProcess(int pid, int C, int F, int S, int P, int Q, double A, double B, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), C_(C), F_(F), S_(S), P_(P), Q_(Q), A_(A), B_(B), bus_(bus), rng_(rng) {}
    
    void initialize(double startTime) override { nextTime_ = startTime + rng_.uniform(A_, B_); }
    double nextEventTime() const override { return nextTime_; }
    void handleEvent(double currentTime) override {
        if (currentTime < nextTime_) return;
        
        // Discard incoming replies since the customer simply loops its requests
        auto &inbox = bus_.netToProc(pid_);
        while (!inbox.empty()) inbox.pop();

        Message m;
        m.time = currentTime; 
        m.sender = pid_; 
        m.receiver = C_ + F_ + rng_.uniformInt(0, S_ - 1); // Random Server PID
        m.item = rng_.uniformInt(1, P_); 
        m.quantity = (double)rng_.uniformInt(1, Q_);
        bus_.procToNet(pid_).push(m);
        
        nextTime_ = currentTime + rng_.uniform(A_, B_);
    }
private:
    int pid_, C_, F_, S_, P_, Q_; double A_, B_, nextTime_; MessageBus &bus_; RandomGenerator &rng_;
};

class SupplierProcess : public DiscreteEventProcess {
public:
    SupplierProcess(int pid, int DB_PID, int P, int Q, double V, double W, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), DB_PID_(DB_PID), P_(P), Q_(Q), V_(V), W_(W), bus_(bus), rng_(rng) {}
    
    void initialize(double startTime) override { nextTime_ = startTime + rng_.uniform(V_, W_); }
    double nextEventTime() const override { return nextTime_; }
    void handleEvent(double currentTime) override {
        if (currentTime < nextTime_) return;
        
        Message m;
        m.time = currentTime; 
        m.sender = pid_; 
        m.receiver = DB_PID_;
        m.item = rng_.uniformInt(1, P_); 
        m.quantity = (double)rng_.uniformInt(1, Q_);
        bus_.procToNet(pid_).push(m); // Direct to Central DB
        
        nextTime_ = currentTime + rng_.uniform(V_, W_);
    }
private:
    int pid_, DB_PID_, P_, Q_; double V_, W_, nextTime_; MessageBus &bus_; RandomGenerator &rng_;
};

class DBProcess : public DiscreteEventProcess {
public:
    DBProcess(int pid, int C, int F, int S, int P, int Q, double l, double s_time, MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), C_(C), F_(F), S_(S), P_(P), Q_(Q), l_(l), s_time_(s_time), bus_(bus), rng_(rng) {
        inv_.assign(P + 1, 0);
    }
    
    void initialize(double startTime) override {
        for(int i=1; i<=P_; ++i) inv_[i] = rng_.uniformInt(0, Q_);
        nextTime_ = startTime + 0.001;
    }
    double nextEventTime() const override { return nextTime_; }
    void handleEvent(double currentTime) override {
        auto &inbox = bus_.netToProc(pid_);
        while (!inbox.empty()) {
            internal_inbox_.push_back(inbox.front());
            inbox.pop();
        }

        if (currentTime < nextTime_) return;

        if (!internal_inbox_.empty()) {
            Message m = internal_inbox_.front();
            internal_inbox_.pop_front();
            
            if (m.sender >= C_ && m.sender < C_ + F_) { // Msg from Supplier
                inv_[m.item] += (int)m.quantity;
                nextTime_ = currentTime + l_; // Takes read time
            } else if (m.sender >= C_ + F_ && m.sender < C_ + F_ + S_) { // Msg from Server
                if (m.quantity == -1) { // -1 identifies a Query request
                    Message rep;
                    rep.time = currentTime;
                    rep.sender = pid_;
                    rep.receiver = m.sender;
                    rep.item = m.item;
                    rep.quantity = inv_[m.item];
                    bus_.procToNet(pid_).push(rep);
                    nextTime_ = currentTime + l_ + s_time_; // read + write time
                } else { // >=0 identifies a Decrement notification
                    inv_[m.item] -= (int)m.quantity;
                    if (inv_[m.item] < 0) inv_[m.item] = 0; // Prevent negatives
                    nextTime_ = currentTime + l_; // read time only
                }
            }
        } else {
            nextTime_ = currentTime + 0.001; // Polling loop
        }
    }
private:
    int pid_, C_, F_, S_, P_, Q_; 
    double l_, s_time_, nextTime_; 
    MessageBus &bus_; RandomGenerator &rng_;
    std::vector<int> inv_;
    std::deque<Message> internal_inbox_;
};

class ServerProcess : public DiscreteEventProcess {
public:
    ServerProcess(int pid, int C, int DB_PID, double z, double v, MessageBus &bus, long long &missed, long long &trans)
        : pid_(pid), C_(C), DB_PID_(DB_PID), z_(z), v_(v), bus_(bus), missed_(missed), trans_(trans), state_(0) {}
    
    void initialize(double startTime) override { nextTime_ = startTime + 0.001; }
    double nextEventTime() const override { return nextTime_; }
    void handleEvent(double currentTime) override {
        auto &inbox = bus_.netToProc(pid_);
        while (!inbox.empty()) {
            internal_inbox_.push_back(inbox.front());
            inbox.pop();
        }

        if (currentTime < nextTime_) return;

        if (state_ == 0) { // IDLE State
            if (!internal_inbox_.empty()) {
                Message m = internal_inbox_.front();
                internal_inbox_.pop_front();
                
                if (m.sender < C_) { // Request from Customer
                    cur_cust_ = m.sender;
                    cur_item_ = m.item;
                    cur_q_ = (int)m.quantity;
                    
                    Message req;
                    req.time = currentTime;
                    req.sender = pid_;
                    req.receiver = DB_PID_;
                    req.item = cur_item_;
                    req.quantity = -1; // -1 marks a DB Query
                    bus_.procToNet(pid_).push(req);
                    
                    state_ = 1; // Transition to WAITING DB
                    nextTime_ = currentTime + z_ + v_; // time to read customer msg + write query
                }
            } else {
                nextTime_ = currentTime + 0.001;
            }
        } else if (state_ == 1) { // WAITING DB State
            // Look explicitly for the reply from DB
            auto it = std::find_if(internal_inbox_.begin(), internal_inbox_.end(),
                [this](const Message& msg) { return msg.sender == DB_PID_; });
                
            if (it != internal_inbox_.end()) {
                Message m = *it;
                internal_inbox_.erase(it);
                
                int g = (int)m.quantity;
                int k = std::min(cur_q_, g);
                
                // --- FEBRUARY METRIC CHANGE ---
                missed_ += (cur_q_ - k); // Missed items
                trans_ += 1;             // Total transactions
                
                // 1. Notify DB to decrement
                Message dec;
                dec.time = currentTime;
                dec.sender = pid_;
                dec.receiver = DB_PID_;
                dec.item = cur_item_;
                dec.quantity = k;
                bus_.procToNet(pid_).push(dec);
                
                // 2. Reply to Customer
                Message rep;
                rep.time = currentTime;
                rep.sender = pid_;
                rep.receiver = cur_cust_;
                rep.item = cur_item_;
                rep.quantity = k;
                bus_.procToNet(pid_).push(rep);
                
                state_ = 0; // Return to IDLE
                nextTime_ = currentTime + z_ + v_ + v_; // time to read reply + 2 output writes
            } else {
                nextTime_ = currentTime + 0.001;
            }
        }
    }
private:
    int pid_, C_, DB_PID_; 
    double z_, v_, nextTime_; 
    MessageBus &bus_; 
    long long &missed_; long long &trans_;
    int state_; // 0=IDLE, 1=WAITING DB
    int cur_cust_, cur_item_, cur_q_;
    std::deque<Message> internal_inbox_;
};

}

int main() {
    SELib::RandomGenerator rng(0);
    SELib::ParameterParser parser;
    if (!parser.parseFile("parameters.txt")) return 1;
    
    // Parse February-Specific parameters
    double H = parser.getDouble("H");
    int M = parser.getInt("M");
    int C = parser.getInt("C");
    double A = parser.getDouble("A");
    double B = parser.getDouble("B");
    int Q = parser.getInt("Q");
    double V = parser.getDouble("V");
    double W = parser.getDouble("W");
    int P = parser.getInt("P");
    int S = parser.getInt("S");
    int F_suppliers = parser.getInt("F");
    
    double l = parser.getDouble("l");
    double s_time = parser.getDouble("s");
    double r = parser.getDouble("r");
    double w = parser.getDouble("w");
    double z = parser.getDouble("z");
    double v = parser.getDouble("v");

    SELib::Statistics stats;
    for (int m = 0; m < M; ++m) {
        int DB_PID = C + F_suppliers + S; // Define DB at the end of the PID range
        SELib::MessageBus bus(DB_PID + 1); 
        SELib::DiscreteEventSystem system; 
        
        long long missed = 0;
        long long trans = 0;
        
        // PIDs 0 to C-1
        for(int i=0; i<C; ++i) 
            system.emplaceProcess<SELib::CustomerProcess>(i, C, F_suppliers, S, P, Q, A, B, bus, rng);
            
        // PIDs C to C+F-1
        for(int i=0; i<F_suppliers; ++i) 
            system.emplaceProcess<SELib::SupplierProcess>(C+i, DB_PID, P, Q, V, W, bus, rng);
            
        // PIDs C+F to C+F+S-1
        for(int i=0; i<S; ++i) 
            system.emplaceProcess<SELib::ServerProcess>(C+F_suppliers+i, C, DB_PID, z, v, bus, missed, trans);
            
        // PID DB_PID
        system.emplaceProcess<SELib::DBProcess>(DB_PID, C, F_suppliers, S, P, Q, l, s_time, bus, rng);
        
        // February's modified NetworkRouter adds r+w delays implicitly
        system.emplaceProcess<SELib::NetworkRouter>(bus, rng, 0.01, r, w);
        
        SELib::DiscreteEventSimulator sim(system);
        sim.run(H);
        
        // February Metric implementation
        double rate = (trans > 0) ? (double)missed / trans : 0.0;
        stats.addSample(rate);
    }
    
    std::ofstream outFile("results.txt");
    outFile << "2026-02-19-Alessandro-Tang-2106357" << std::endl;
    outFile << "R " << std::fixed << std::setprecision(6) << stats.mean() << std::endl;
    
    return 0;
}