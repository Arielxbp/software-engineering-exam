#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>
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
    CustomerProcess(int pid, int serverBasePid,
                    int S, int P, int Q,
                    double A, double B,
                    double S0, double P0, double Q0,
                    MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), serverBasePid_(serverBasePid),
          S_(S), P_(P), Q_(Q),
          A_(A), B_(B), S0_(S0), P0_(P0), Q0_(Q0),
          bus_(bus), rng_(rng), nextTime_(0.0) {}

    void initialize(double startTime) override {
        nextTime_ = startTime + rng_.uniform(A_, B_);
    }

    double nextEventTime() const override { return nextTime_; }

    void handleEvent(double /*currentTime*/) override {
        // All three selections use 1-based indices, uniform within the chosen class.
        // FIX #4: was 0-based, reversed even/odd, could produce index -1.
        int s = rng_.bernoulli(S0_) ? pickEven(S_, rng_) : pickOdd(S_, rng_);
        int i = rng_.bernoulli(P0_) ? pickEven(P_, rng_) : pickOdd(P_, rng_);
        int q = rng_.bernoulli(Q0_) ? pickEven(Q_, rng_) : pickOdd(Q_, rng_);

        Message m;
        m.time     = nextTime_;            // send time = when this event fires
        m.sender   = pid_;
        m.receiver = serverBasePid_ + (s - 1);   // 1-based s → 0-offset PID
        m.item     = i;
        m.quantity = static_cast<double>(q);
        bus_.procToNet(pid_).push(m);

        nextTime_ += rng_.uniform(A_, B_);
    }

private:
    int    pid_, serverBasePid_, S_, P_, Q_;
    double A_, B_, S0_, P0_, Q0_;
    MessageBus      &bus_;
    RandomGenerator &rng_;
    double nextTime_;
};

// ---------------------------------------------------------------------------
// SupplierProcess
// ---------------------------------------------------------------------------
class SupplierProcess : public DiscreteEventProcess {
public:
    SupplierProcess(int pid, int databasePid,
                    int P, int Q,
                    double V, double W,
                    MessageBus &bus, RandomGenerator &rng)
        : pid_(pid), databasePid_(databasePid),
          P_(P), Q_(Q), V_(V), W_(W),
          bus_(bus), rng_(rng), nextTime_(0.0) {}

    void initialize(double startTime) override {
        nextTime_ = startTime + rng_.uniform(V_, W_);
    }

    double nextEventTime() const override { return nextTime_; }

    void handleEvent(double /*currentTime*/) override {
        Message m;
        m.time     = nextTime_;
        m.sender   = pid_;
        m.receiver = databasePid_;
        m.item     = rng_.uniformInt(1, P_);
        m.quantity = static_cast<double>(rng_.uniformInt(1, Q_));
        bus_.procToNet(pid_).push(m);

        nextTime_ += rng_.uniform(V_, W_);
    }

private:
    int    pid_, databasePid_, P_, Q_;
    double V_, W_;
    MessageBus      &bus_;
    RandomGenerator &rng_;
    double nextTime_;
};

// ---------------------------------------------------------------------------
// ServerProcess
// ---------------------------------------------------------------------------
class ServerProcess : public DiscreteEventProcess {
public:
    // serverIdx: 1-based index (1..S).  Even → T_=ST0, Odd → T_=ST1.
    ServerProcess(int pid, int serverIdx,
                  int databasePid, int P, int Q,
                  double ST0, double ST1,
                  double SEP, double SOP,
                  double QEA, double QOA,
                  double r,
                  const std::vector<int> &costs,   // costs[1..P]
                  MessageBus &bus, RandomGenerator &rng)
        : pid_(pid),
          isEven_((serverIdx % 2) == 0),
          T_(isEven_ ? ST0 : ST1),
          databasePid_(databasePid), P_(P), Q_(Q),
          SEP_(SEP), SOP_(SOP), QEA_(QEA), QOA_(QOA),
          r_(r), costs_(costs),
          bus_(bus), rng_(rng),
          nextCacheUpdateTime_(0.0),
          sell_buy_(0.0), y_(0.0)
    {
        cache_.assign(static_cast<std::size_t>(P_ + 1), {0, 0}); // 1-based
    }

    void initialize(double startTime) override {
        for (int i = 1; i <= P_; ++i)
            cache_[static_cast<std::size_t>(i)].second = costs_[static_cast<std::size_t>(i)];
        sell_buy_ = 0.0;
        y_        = 0.0;
        // FIX #5: was using uninitialised lastTime_; now use absolute nextCacheUpdateTime_.
        nextCacheUpdateTime_ = startTime + T_;
    }

    // FIX (independent timers): nextEventTime = min(cache-update timer, next
    // inbox message becoming ready).  Peeking the queue front is O(1).
    double nextEventTime() const override {
        double t = nextCacheUpdateTime_;
        const auto &inbox = bus_.netToProc(pid_);
        if (!inbox.empty())
            t = std::min(t, inbox.front().time + r_);
        return t;
    }

    void handleEvent(double currentTime) override {
        // --- 1. Process every inbox message whose r-delay has expired ---
        // FIX #7: was only popping one message; now a while-loop.
        // r-delay is enforced here via m.time + r_ (router runs fast/instant).
        auto &inbox = bus_.netToProc(pid_);
        while (!inbox.empty() && currentTime >= inbox.front().time + r_) {
            Message m = inbox.front();
            inbox.pop();

            if (m.sender == databasePid_) {
                // FIX #6: was updating cache immediately on send (before reply).
                // Now we only update when the DB reply actually arrives.
                int qty  = static_cast<int>(m.quantity);
                int item = m.item;
                cache_[static_cast<std::size_t>(item)].first += qty;
                sell_buy_ -= qty * static_cast<double>(
                    cache_[static_cast<std::size_t>(item)].second);
            } else {
                // Customer request (i, q)
                int item  = m.item;
                int g     = cache_[static_cast<std::size_t>(item)].first;
                int v     = cache_[static_cast<std::size_t>(item)].second;
                int k     = std::min(g, static_cast<int>(m.quantity));
                cache_[static_cast<std::size_t>(item)].first -= k;
                sell_buy_ += k * static_cast<double>(v);

                // Reply (i, k, v) to customer -- v is implicit, k in quantity
                Message reply;
                reply.time     = currentTime;
                reply.sender   = pid_;
                reply.receiver = m.sender;
                reply.item     = item;
                reply.quantity = static_cast<double>(k);
                bus_.procToNet(pid_).push(reply);
            }
        }

        // --- 2. Cache-update timer ---
        if (currentTime >= nextCacheUpdateTime_) {
            // Update storage cost y(s) before refreshing cache (uses current holdings).
            // FIX #2: y(s) was never computed.
            // Formula: y(s, t+T) = y(s, t) + T * 0.1 * Σ α(s,i)  where α(s,i) = q*v
            double storageSum = 0.0;
            for (int i = 1; i <= P_; ++i) {
                auto &entry = cache_[static_cast<std::size_t>(i)];
                storageSum += static_cast<double>(entry.first) *
                              static_cast<double>(entry.second);
            }
            y_ += T_ * 0.1 * storageSum;

            // Select item: even server → SEP prob of even i; odd → SOP prob of odd i
            // FIX #4: was using pid_ parity (PID 100 = server 1 = odd, backwards).
            int item = isEven_
                ? (rng_.bernoulli(SEP_) ? pickEven(P_, rng_) : pickOdd(P_, rng_))
                : (rng_.bernoulli(SOP_) ? pickOdd(P_,  rng_) : pickEven(P_, rng_));

            // Select quantity: even server → QEA prob of even q; odd → QOA prob of odd q
            int qty = isEven_
                ? (rng_.bernoulli(QEA_) ? pickEven(Q_, rng_) : pickOdd(Q_, rng_))
                : (rng_.bernoulli(QOA_) ? pickOdd(Q_,  rng_) : pickEven(Q_, rng_));

            Message req;
            req.time     = currentTime;
            req.sender   = pid_;
            req.receiver = databasePid_;
            req.item     = item;
            req.quantity = static_cast<double>(qty);
            bus_.procToNet(pid_).push(req);

            nextCacheUpdateTime_ = currentTime + T_;
        }
    }

    // FIX #1: expose values so main() can compute the reward rate.
    double getSellBuy() const { return sell_buy_; }
    double getY()       const { return y_;        }

private:
    int    pid_;
    bool   isEven_;
    double T_;                              // ST0 (even) or ST1 (odd)
    int    databasePid_, P_, Q_;
    double SEP_, SOP_, QEA_, QOA_, r_;
    std::vector<int>              costs_;  // unit costs per product, 1-based
    std::vector<std::pair<int,int>> cache_; // (quantity, unit_cost), 1-based
    MessageBus      &bus_;
    RandomGenerator &rng_;
    double nextCacheUpdateTime_;
    double sell_buy_;
    double y_;
};

// ---------------------------------------------------------------------------
// DatabaseProcess
// ---------------------------------------------------------------------------
class DatabaseProcess : public DiscreteEventProcess {
public:
    // supplierPidMin / supplierPidMax: inclusive range of supplier PIDs,
    // used to distinguish supplier messages from server messages.
    DatabaseProcess(int pid, int supplierPidMin, int supplierPidMax,
                    double r,
                    std::vector<std::pair<int,int>> &inv,  // (qty, unit_cost), 1-based
                    MessageBus &bus)
        : pid_(pid),
          supplierPidMin_(supplierPidMin), supplierPidMax_(supplierPidMax),
          r_(r), inv_(inv), bus_(bus) {}

    void initialize(double /*startTime*/) override {}  // no self-scheduled timer

    // Wake up exactly when the front inbox message becomes ready (send_time + r).
    double nextEventTime() const override {
        const auto &inbox = bus_.netToProc(pid_);
        if (!inbox.empty())
            return inbox.front().time + r_;
        return std::numeric_limits<double>::infinity();
    }

    void handleEvent(double currentTime) override {
        auto &inbox = bus_.netToProc(pid_);
        while (!inbox.empty() && currentTime >= inbox.front().time + r_) {
            Message m = inbox.front();
            inbox.pop();

            bool fromSupplier = (m.sender >= supplierPidMin_ &&
                                 m.sender <= supplierPidMax_);

            if (fromSupplier) {
                // Increment DB stock; send ack back to supplier.
                inv_[static_cast<std::size_t>(m.item)].first +=
                    static_cast<int>(m.quantity);

                Message ack;
                ack.time     = currentTime;
                ack.sender   = pid_;
                ack.receiver = m.sender;
                ack.item     = m.item;
                ack.quantity = 0.0;
                bus_.procToNet(pid_).push(ack);
            } else {
                // Server cache-update request: reply (i, k, v).
                // v is in inv_[i].second; k = min(requested, available).
                int req       = static_cast<int>(m.quantity);
                int available = std::min(req,
                    inv_[static_cast<std::size_t>(m.item)].first);
                inv_[static_cast<std::size_t>(m.item)].first -= available;

                Message reply;
                reply.time     = currentTime;
                reply.sender   = pid_;
                reply.receiver = m.sender;
                reply.item     = m.item;
                reply.quantity = static_cast<double>(available);
                bus_.procToNet(pid_).push(reply);
            }
        }
    }

private:
    int  pid_, supplierPidMin_, supplierPidMax_;
    double r_;
    std::vector<std::pair<int,int>> &inv_;
    MessageBus &bus_;
};

} // namespace SELib

// ===========================================================================
// main
// ===========================================================================
int main() {
    SELib::RandomGenerator rng(std::random_device{}());
    SELib::ParameterParser parser;
    if (!parser.parseFile("parameters.txt")) return 1;

    const double H   = parser.getDouble("H");
    const int    M   = parser.getInt("M");
    const int    C   = parser.getInt("C");
    const int    S   = parser.getInt("S");
    const int    F   = parser.getInt("F");
    const double A   = parser.getDouble("A");
    const double B   = parser.getDouble("B");
    const double V   = parser.getDouble("V");
    const double W   = parser.getDouble("W");
    const int    P   = parser.getInt("P");
    const int    Q   = parser.getInt("Q");
    const double r   = parser.getDouble("r");
    const double S0  = parser.getDouble("S0");
    const double P0  = parser.getDouble("P0");
    const double Q0  = parser.getDouble("Q0");
    const double ST0 = parser.getDouble("ST0");
    const double ST1 = parser.getDouble("ST1");
    const double SEP = parser.getDouble("SEP");
    const double SOP = parser.getDouble("SOP");
    const double QEA = parser.getDouble("QEA");
    const double QOA = parser.getDouble("QOA");

    // PID layout (contiguous, no gaps):
    //   Customers  : 0        .. C-1
    //   Suppliers  : C        .. C+F-1
    //   Servers    : C+F      .. C+F+S-1
    //   Database   : C+F+S
    // FIX (structural): was hardcoded to 199; bus sized to max_PID + 1.
    const int serverBasePid  = C + F;
    const int databasePid    = C + F + S;
    const int busSize        = C + F + S + 1;

    SELib::Statistics stats;

    for (int sim = 0; sim < M; ++sim) {
        SELib::MessageBus         bus(static_cast<std::size_t>(busSize));
        SELib::DiscreteEventSystem system;

        // Initialise inventory: each product gets random qty in [0,Q]
        // and a random unit cost in [1,100].  Same values propagated to servers.
        std::vector<std::pair<int,int>> inv(static_cast<std::size_t>(P + 1), {0, 0});
        std::vector<int>               costs(static_cast<std::size_t>(P + 1), 0);
        for (int i = 1; i <= P; ++i) {
            inv[static_cast<std::size_t>(i)].first  = rng.uniformInt(0, Q);
            inv[static_cast<std::size_t>(i)].second = rng.uniformInt(1, 100);
            costs[static_cast<std::size_t>(i)]      = inv[static_cast<std::size_t>(i)].second;
        }

        // --- Customers ---
        for (int i = 0; i < C; ++i)
            system.emplaceProcess<SELib::CustomerProcess>(
                i, serverBasePid, S, P, Q, A, B, S0, P0, Q0, bus, rng);

        // --- Suppliers ---
        for (int i = 0; i < F; ++i)
            system.emplaceProcess<SELib::SupplierProcess>(
                C + i, databasePid, P, Q, V, W, bus, rng);

        // --- Servers (store refs to read SellBuy / y after run) ---
        // FIX #1/#2: collect server references so reward rate can be computed.
        std::vector<std::reference_wrapper<SELib::ServerProcess>> serverRefs;
        for (int i = 0; i < S; ++i) {
            auto &sp = system.emplaceProcess<SELib::ServerProcess>(
                serverBasePid + i,
                i + 1,           // 1-based serverIdx: FIX #4
                databasePid, P, Q,
                ST0, ST1, SEP, SOP, QEA, QOA,
                r, costs, bus, rng);
            serverRefs.emplace_back(std::ref(sp));
        }

        // --- Database ---
        system.emplaceProcess<SELib::DatabaseProcess>(
            databasePid,
            C,           // supplierPidMin
            C + F - 1,   // supplierPidMax
            r, inv, bus);

        // --- Router ---
        // scanPeriod kept small so the router delivers messages almost instantly.
        // The actual r-delay is enforced at receive time via (m.time + r) checks
        // in nextEventTime() / handleEvent(), not via the router period.
        // Setting scanPeriod = r would be wrong: with N processes a full cycle
        // takes N*r, far exceeding the intended single-message delay of r.
        system.emplaceProcess<SELib::NetworkRouter>(bus, rng, 0.0001);

        SELib::DiscreteEventSimulator des(system);
        des.run(H);

        // FIX #1: compute RR(H) = Σ_s (β(s,H) - y(s,H)) / H
        // FIX #3: output key is "RR", not "R".
        double totalSellBuy = 0.0;
        double totalY       = 0.0;
        for (auto &ref : serverRefs) {
            totalSellBuy += ref.get().getSellBuy();
            totalY       += ref.get().getY();
        }
        stats.addSample((totalSellBuy - totalY) / H);
    }

    std::ofstream out("results.txt");
    out << "2026-06-02-Alessandro-Tang-2106357\n";
    out << "RR " << std::fixed << std::setprecision(6) << stats.mean() << "\n";
    return 0;
}