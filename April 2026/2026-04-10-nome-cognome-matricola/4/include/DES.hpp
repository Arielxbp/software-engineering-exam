
#pragma once

#include "Random.hpp"
#include <algorithm>
#include <limits>
#include <memory>
#include <queue>
#include <stdexcept>
#include <vector>

namespace SELib {

// ---------------------------------------------------------------------------
// Messaggio
// ---------------------------------------------------------------------------
struct Message {
  double time{0.0};
  int sender{-1};
  int receiver{-1};
  int item{0};
  double quantity{0.0};
};

// ---------------------------------------------------------------------------
// Bus di messaggi: process -> network, network -> process
// ---------------------------------------------------------------------------
class MessageBus {
public:
  explicit MessageBus(std::size_t n = 0) { resize(n); }

  void resize(std::size_t n) {
    procToNet_.assign(n, {});
    netToProc_.assign(n, {});
  }

  std::size_t size() const { return procToNet_.size(); }

  std::queue<Message> &procToNet(int pid) { return procToNet_.at(pid); }
  std::queue<Message> &netToProc(int pid) { return netToProc_.at(pid); }
  const std::queue<Message> &procToNet(int pid) const {
    return procToNet_.at(pid);
  }
  const std::queue<Message> &netToProc(int pid) const {
    return netToProc_.at(pid);
  }

private:
  std::vector<std::queue<Message>> procToNet_;
  std::vector<std::queue<Message>> netToProc_;
};

// ---------------------------------------------------------------------------
// Processo base a eventi discreti
// ---------------------------------------------------------------------------
class DiscreteEventProcess {
public:
  virtual ~DiscreteEventProcess() = default;
  virtual void initialize(double startTime) = 0;
  virtual double nextEventTime() const = 0;
  virtual void handleEvent(double currentTime) = 0;
};

// ---------------------------------------------------------------------------
// NetworkRouter: inoltra i messaggi P->N->P
// ---------------------------------------------------------------------------
class NetworkRouter : public DiscreteEventProcess {
public:
  NetworkRouter(MessageBus &bus, RandomGenerator &rng, double scanPeriod = 0.1, double deliveryDelay = 0.0)
      : bus_(bus), rng_(rng), scanPeriod_(scanPeriod), deliveryDelay_(deliveryDelay), time_(0.0),
        nextScanTime_(0.0) {}

  void initialize(double startTime) override {
    time_ = startTime;
    nextScanTime_ = startTime + scanPeriod_;
    inTransit_.clear();

    const int n = static_cast<int>(bus_.size());
    if (n <= 0) {
      throw std::runtime_error(
          "NetworkRouter: no processes configured in MessageBus");
    }

    scanner_.resize(n);
    for (int i = 0; i < n; ++i) {
      scanner_[i] = i;
    }
    reshuffleScanner();
  }

  double nextEventTime() const override {
    double t = nextScanTime_;
    for (auto &im : inTransit_) {
      if (im.deliveryTime < t) {
        t = im.deliveryTime;
      }
    }
    return t;
  }

  void handleEvent(double currentTime) override {

    deliverReady(currentTime);

    if (currentTime < nextScanTime_)
      return;

    const int n = static_cast<int>(scanner_.size());
    if (n == 0)
      return;

    reshuffleScanner();

    for (int idx = 0; idx < n; ++idx) {
      int channel = scanner_[idx];
      auto &fromQueue = bus_.procToNet(channel);

      while (!fromQueue.empty()) {
        Message m = fromQueue.front();
        fromQueue.pop();

        if (m.receiver < 0 || m.receiver >= static_cast<int>(bus_.size())) {
          throw std::runtime_error("NetworkRouter: invalid receiver index");
        }

        if (deliveryDelay_ <= 0.0) {
          bus_.netToProc(m.receiver).push(m);
        } else {
          inTransit_.push_back({m, currentTime + deliveryDelay_});
        }
      }
    }

    nextScanTime_ = currentTime + scanPeriod_;
  }

private:

  struct InTransitMessage {
    Message msg;
    double deliveryTime; // currentTime al prelievo + deliveryDelay_
  };

  void deliverReady(double currentTime) {
    auto it = inTransit_.begin();
    while (it != inTransit_.end()) {
      if (it->deliveryTime <= currentTime) {
        bus_.netToProc(it->msg.receiver).push(it->msg);
        it = inTransit_.erase(it);
      } else {
        ++it;
      }
    }
  }

  void reshuffleScanner() {
    auto &eng = rng_.engine(); // serve engine() in RandomGenerator
    std::shuffle(scanner_.begin(), scanner_.end(), eng);
  }

  MessageBus &bus_;
  RandomGenerator &rng_;

  double scanPeriod_;
  double deliveryDelay_;
  double time_;
  double nextScanTime_;
  std::vector<int> scanner_;
  std::vector<InTransitMessage> inTransit_;
};

// ---------------------------------------------------------------------------
// Sistema DES: collezione di processi
// ---------------------------------------------------------------------------
class DiscreteEventSystem {
public:
  template <typename Proc, typename... Args>
  Proc &emplaceProcess(Args &&...args) {
    auto ptr = std::make_unique<Proc>(std::forward<Args>(args)...);
    Proc &ref = *ptr;
    processes_.push_back(std::move(ptr));
    return ref;
  }

  void initialize(double startTime = 0.0) {
    for (auto &p : processes_) {
      p->initialize(startTime);
    }
  }

  double nextEventTime() const {
    if (processes_.empty()) {
      return std::numeric_limits<double>::infinity();
    }

    double tMin = std::numeric_limits<double>::infinity();
    for (const auto &p : processes_) {
      double t = p->nextEventTime();
      if (t < tMin)
        tMin = t;
    }
    return tMin;
  }

  void handleEvent(double currentTime) {
    for (auto &p : processes_) {
      if (p->nextEventTime() <= currentTime) {
        p->handleEvent(currentTime);
      }
    }
  }

private:
  std::vector<std::unique_ptr<DiscreteEventProcess>> processes_;
};

// ---------------------------------------------------------------------------
// Simulatore DES
// ---------------------------------------------------------------------------
class DiscreteEventSimulator {
public:
  explicit DiscreteEventSimulator(DiscreteEventSystem &system)
      : system_(system), currentTime_(0.0) {}

  void run(double horizon) {
    system_.initialize(0.0);
    currentTime_ = 0.0;

    while (currentTime_ < horizon) {
      double nextTime = system_.nextEventTime();
      if (nextTime == std::numeric_limits<double>::infinity())
        break;
      currentTime_ = nextTime;
      system_.handleEvent(currentTime_);
    }
  }

  double currentTime() const { return currentTime_; }

private:
  DiscreteEventSystem &system_;
  double currentTime_;
};

} // namespace SELib
