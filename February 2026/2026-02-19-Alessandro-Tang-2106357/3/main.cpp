#include <fstream>
#include <iomanip>
#include <algorithm>
#include <queue>
#include <vector>
#include "include/DES.hpp"
#include "include/IO.hpp"
#include "include/Random.hpp"
#include "include/Stat.hpp"

// ---------------------------------------------------------------------------
// PID layout
//   Customers : 0 .. C-1
//   Suppliers : C .. C+F-1
//   Servers   : 100 .. 100+S-1
//   DB        : 200
// ---------------------------------------------------------------------------
static constexpr int DB_PID = 200;

namespace SELib {

// ---------------------------------------------------------------------------
// CustomerProcess
// Markov chain with one state. Stays Uniform[A,B] then sends (i,q) to a
// random server. nextTime accounts for the w write-to-network cost.
// ---------------------------------------------------------------------------
class CustomerProcess : public DiscreteEventProcess {
public:
    CustomerProcess(int pid, int S, int P, int Q, double A, double B, double w,
                    MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), S_(S), P_(P), Q_(Q), A_(A), B_(B),
          nextTime_(0.0), bus_(bus), rng_(rng) {}

    void initialize(double startTime) override {
        nextTime_ = startTime + rng_.uniform(A_, B_);
    }
    double nextEventTime() const override { return nextTime_; }
    void handleEvent(double currentTime) override {
        Message m;
        m.time     = currentTime;
        m.sender   = pid_;
        m.receiver = 100 + rng_.uniformInt(0, S_ - 1);  // random server
        m.item     = rng_.uniformInt(1, P_);
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

// ---------------------------------------------------------------------------
// SupplierProcess
// Markov chain with one state. Stays Uniform[V,W] then sends (i,q) directly
// to the centralised DB.
// ---------------------------------------------------------------------------
class SupplierProcess : public DiscreteEventProcess {
public:
    SupplierProcess(int pid, int P, int Q, double V, double W, double w,
                    MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), P_(P), Q_(Q), V_(V), W_(W),
          nextTime_(0.0), bus_(bus), rng_(rng) {}

    void initialize(double startTime) override {
        nextTime_ = startTime + rng_.uniform(V_, W_);
    }
    double nextEventTime() const override { return nextTime_; }
    void handleEvent(double currentTime) override {
        Message m;
        m.time     = currentTime;
        m.sender   = pid_;
        m.receiver = DB_PID;
        m.item     = rng_.uniformInt(1, P_);
        m.quantity = (double)rng_.uniformInt(1, Q_);  // q in {1..Q}
        bus_.procToNet(pid_).push(m);
        nextTime_ = currentTime + rng_.uniform(V_, W_);
    }
private:
    int pid_, P_, Q_;
    double V_, W_, nextTime_;
    MessageBus &bus_;
    RandomGenerator &rng_;
};

// ---------------------------------------------------------------------------
// DBProcess — centralised database
//
// inv_[1..P] initialised at random in [0,Q].
//
// Incoming messages:
//   From supplier (C <= sender < C+F):
//       supply  → inv_[item] += quantity
//   From server (100 <= sender < 100+S):
//       quantity == 0  → query:  reply with g = inv_[item]
//       quantity  > 0  → update: inv_[item] -= k
//
// Latency: +l when a message is read, +s when a response is written.
// ---------------------------------------------------------------------------
class DBProcess : public DiscreteEventProcess {
public:
    DBProcess(int P, int Q, int C, int F, int S,
              double l, double s,
              MessageBus &bus, RandomGenerator &rng)
        : P_(P), Q_(Q), C_(C), F_(F), S_(S),
          l_(l), s_(s), nextTime_(0.0), bus_(bus), rng_(rng) {
        inv_.assign(P + 1, 0);
    }

    void initialize(double startTime) override {
        for (int i = 1; i <= P_; ++i)
            inv_[i] = rng_.uniformInt(0, Q_);
        nextTime_ = startTime + l_;   // first poll: just read cost
    }
    double nextEventTime() const override { return nextTime_; }
    void handleEvent(double currentTime) override {
        auto &inbox = bus_.netToProc(DB_PID);
        bool read  = false;
        bool wrote = false;
        while (!inbox.empty()) {
            Message m = inbox.front(); inbox.pop();
            read = true;

            bool fromSupplier = (m.sender >= C_ && m.sender < C_ + F_);
            bool fromServer   = (m.sender >= 100 && m.sender < 100 + S_);

            if (fromSupplier) {
                inv_[m.item] += (int)m.quantity;

            } else if (fromServer) { 
                if (m.quantity == 0.0) {
                    // Query: respond with current stock
                    Message resp;
                    resp.time     = currentTime;
                    resp.sender   = DB_PID;
                    resp.receiver = m.sender;
                    resp.item     = m.item;
                    resp.quantity = (double)inv_[m.item];
                    bus_.procToNet(DB_PID).push(resp);
                    wrote = true;
                } else {
                    // Update: decrement stock by k (no write)
                    int k = (int)m.quantity;
                    inv_[m.item] = std::max(0, inv_[m.item] - k);
                }
            }
        }
        // add l only if a message was read, s+w only if a message was written
        nextTime_ = currentTime + (read ? l_ : 0.0) + (wrote ? s_ : 0.0);
    }
private:
    int P_, Q_, C_, F_, S_;
    double l_, s_, nextTime_;
    MessageBus &bus_;
    RandomGenerator &rng_;
    std::vector<int> inv_;
};

// ---------------------------------------------------------------------------
// ServerProcess
//
// Protocol per transaction:
//   1. Receive (i,q) from customer j  → store in pending queue
//   2. Send DB query: (item=i, quantity=0)
//   3. Receive DB reply (item=i, quantity=g)
//   4. k = min(q, g); missed_sales += (q-k); transactions++
//   5. Send DB update: (item=i, quantity=k)  [if k>0]
//   6. Send reply (item=i, quantity=k) to customer j
//
// Latency: +z when a message is read, then +v when a message is written.
// ---------------------------------------------------------------------------
class ServerProcess : public DiscreteEventProcess {
public:
    ServerProcess(int pid, int C, int F,
                  double z, double v,
                  MessageBus &bus,
                  long long &missedSales, long long &transactions)
        : pid_(pid), C_(C), F_(F),
          z_(z), v_(v), nextTime_(0.0), bus_(bus),
          missedSales_(missedSales), transactions_(transactions) {}

    void initialize(double startTime) override {
        nextTime_ = startTime;   // first poll: just read cost
    }
    double nextEventTime() const override { return nextTime_; }
    void handleEvent(double currentTime) override {
        auto &inbox = bus_.netToProc(pid_);
        bool read  = false;
        bool wrote = false;
        while (!inbox.empty()) {
            Message m = inbox.front(); inbox.pop();
            read = true;

            bool fromCustomer = (m.sender >= 0   && m.sender < C_);
            bool fromDB       = (m.sender == DB_PID);

            if (fromCustomer) {

                pending_.push({m.sender, m.item, (int)m.quantity});
                Message query;
                query.time     = currentTime;
                query.sender   = pid_;
                query.receiver = DB_PID;
                query.item     = m.item;
                query.quantity = 0.0;   // signals a read query
                bus_.procToNet(pid_).push(query);
                wrote = true;

            } else if (fromDB && !pending_.empty()) {

                auto req = pending_.front(); pending_.pop();
                int g = (int)m.quantity;
                int k = std::min(req.q, g);
                missedSales_  += (req.q - k);
                transactions_ += 1;

                if (k > 0) {
                    Message upd;
                    upd.time     = currentTime;
                    upd.sender   = pid_;
                    upd.receiver = DB_PID;
                    upd.item     = req.item;
                    upd.quantity = (double)k;
                    bus_.procToNet(pid_).push(upd);
                }

                Message resp;
                resp.time     = currentTime;
                resp.sender   = pid_;
                resp.receiver = req.customerPid;
                resp.item     = req.item;
                resp.quantity = (double)k;
                bus_.procToNet(pid_).push(resp);
                wrote = true;
            }
        }
        // add z only if a message was read, v only if a message was written
        nextTime_ = currentTime + (read ? z_ : 0.0) + (wrote ? v_ : 0.0);
    }
private:
    struct Pending { int customerPid, item, q; };
    int pid_, C_, F_;
    double z_, v_, nextTime_;
    MessageBus &bus_;
    long long &missedSales_;
    long long &transactions_;
    std::queue<Pending> pending_;
};

} // namespace SELib

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main() {
    SELib::RandomGenerator rng(std::random_device{}());
    SELib::ParameterParser parser;
    if (!parser.parseFile("parameters.txt")) return 1;

    double H = parser.getDouble("H");
    int M = parser.getInt("M");
    int C = parser.getInt("C");
    int S = parser.getInt("S");
    int F = parser.getInt("F");
    int P = parser.getInt("P");
    int Q = parser.getInt("Q");
    double A = parser.getDouble("A");
    double B = parser.getDouble("B");
    double V = parser.getDouble("V");
    double W = parser.getDouble("W");
    double r = parser.getDouble("r");   // network read time
    double w = parser.getDouble("w");   // network write time
    double l = parser.getDouble("l");   // DB read time
    double s = parser.getDouble("s");   // DB write time
    double z = parser.getDouble("z");   // server read time
    double v = parser.getDouble("v");   // server write time

    SELib::Statistics stats;

    for (int sim = 0; sim < M; ++sim) {
        SELib::MessageBus bus(DB_PID + 1);
        SELib::DiscreteEventSystem system;
        long long missedSales  = 0;
        long long transactions = 0;

        for (int i = 0; i < C; ++i)
            system.emplaceProcess<SELib::CustomerProcess>(
                i, S, P, Q, A, B, w, bus, rng);

        for (int i = 0; i < F; ++i)
            system.emplaceProcess<SELib::SupplierProcess>(
                C + i, P, Q, V, W, w, bus, rng);

        for (int i = 0; i < S; ++i)
            system.emplaceProcess<SELib::ServerProcess>(
                100 + i, C, F, z, v, bus, missedSales, transactions);

        system.emplaceProcess<SELib::DBProcess>(
            P, Q, C, F, S, l, s, bus, rng);

        system.emplaceProcess<SELib::NetworkRouter>(bus, rng, 0.01, r, w);

        SELib::DiscreteEventSimulator runner(system);
        runner.run(H);

        double rate = (transactions > 0)
                      ? (double)missedSales / (double)transactions
                      : 0.0;
        stats.addSample(rate);
    }

    std::ofstream outFile("results.txt");
    outFile << "2026-02-19-Alessandro-Tang-2106357" << std::endl;
    outFile << "R " << std::fixed << std::setprecision(6) << stats.mean() << std::endl;
    return 0;
}
