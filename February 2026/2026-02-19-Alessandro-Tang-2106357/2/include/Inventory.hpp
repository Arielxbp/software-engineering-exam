#pragma once

#include "DES.hpp"
#include "Random.hpp"
#include <cstddef>
#include <vector>

namespace SELib {
// ============================================================================
// 16. CUSTOMER MODEL
// ============================================================================

// Customer: Represents a customer model in a simulation.
// Purpose:
//  - Simulates customer requests over time.
//  - Generates random requests for products at specific intervals.
// Usage:
//  Customer customer(5.0, 10.0, 3, rng);
//  int productId = customer.step(0.1); // Simulates a time step of 0.1 seconds.
class Customer {
private:
  double nextRequestTime;
  double minInterval;
  double maxInterval;
  int numProducts;
  RandomGenerator &rng;

public:
  Customer(double A, double B, int N, RandomGenerator &generator)
      : minInterval(A), maxInterval(B), numProducts(N), rng(generator) {
    nextRequestTime = rng.uniform(A, B);
  }

  int step(double dt) {
    nextRequestTime -= dt;
    if (nextRequestTime <= 0) {
      nextRequestTime = rng.uniform(minInterval, maxInterval);
      return rng.uniformInt(1, numProducts);
    }
    return 0;
  }
};
class Customer_Event_Driven {
private:
  double nextRequestTime;
  double minInterval;
  double maxInterval;
  int numProducts;
  RandomGenerator &rng;

public:
  Customer_Event_Driven(double A, double B, int N, RandomGenerator &generator,
                        double nextRequestTime)
      : minInterval(A), maxInterval(B), numProducts(N), rng(generator),
        nextRequestTime(nextRequestTime) {}

  int step_event_driven(double *current_time) {
    if (*current_time >= nextRequestTime) {
      nextRequestTime = *current_time + rng.uniform(minInterval, maxInterval);
      return rng.uniformInt(1, numProducts);
    }
    return 0;
  }

  double getNextRequestTime() { return nextRequestTime; };
};

// ============================================================================
// 17. SERVER MODEL
// ============================================================================

// Server: Represents a server in a simulated environment.
// Purpose:
//  - Handles product requests and manages inventory.
//  - Simulates server busy time during request processing.
// Usage:
//  Server server(numProducts, maxInventory, processingTimeNormal,
//  processingTimeSupplier, rng); int result =
//  server.processRequest(productId, needsSupply);
//  server.addInventory(productId, quantityAdded);
class Server {
private:
  std::vector<int> inventory;
  int busyTime;
  int processingTimeNormal;
  int processingTimeSupplier;
  RandomGenerator *rng;

public:
  Server(int numProducts, int K, int F, int G, RandomGenerator &rng)
      : inventory(numProducts + 1, 0), busyTime(0), processingTimeNormal(F),
        processingTimeSupplier(G), rng(&rng) {
    for (int i = 1; i <= numProducts; i++) {
      inventory[i] = rng.uniformInt(0, K);
    }
  }

  bool isBusy() const { return busyTime > 0; }

  void decrementBusyTime() {
    if (busyTime > 0)
      busyTime--;
  }

  int processRequest(int productId, bool needsSupply) {
    if (inventory[productId] > 0) {
      inventory[productId]--;
      busyTime = needsSupply ? processingTimeSupplier : processingTimeNormal;
      return productId;
    } else {
      busyTime = needsSupply ? processingTimeSupplier : processingTimeNormal;
      return -productId;
    }
  }

  int processRequest2(int productId, double supply_prob, int min_prod_refill,
                      int max_prod_refill) {
    if (inventory[productId] > 0) {
      inventory[productId]--;
      busyTime = processingTimeNormal;
      return productId;
    } else {
      if (rng->bernoulli(supply_prob)) {
        int refill_amount = rng->uniformInt(min_prod_refill, max_prod_refill);
        inventory[productId] += refill_amount;
      }
      busyTime = processingTimeSupplier;
      return -productId;
    }
  }

  void addInventory(int productId, int quantity) {
    inventory[productId] += quantity;
  }

  int getInventory(int productId) const { return inventory[productId]; }
};
// ============================================================================
// 17.5 SERVER-EVENT-DRIVEN
// ============================================================================
class Server_Event_Driven {
private:
  std::vector<int> inventory;
  RandomGenerator *rng;
  double next_step_time;
  double min_interval;
  double max_interval;

public:
  Server_Event_Driven(int numProducts, int K, double min_interval,
                      double max_interval, RandomGenerator &rng,
                      double nextStepTime)
      : inventory(numProducts + 1, 0), rng(&rng), min_interval(min_interval),
        max_interval(max_interval), next_step_time(nextStepTime) {
    for (int i = 1; i <= numProducts; i++) {
      inventory[i] = rng.uniformInt(0, K);
    }
  }
  int processRequestEventDriven(double current_time, int productId,
                                double supply_prob, int min_prod_refill,
                                int max_prod_refill) {
    if (current_time >= next_step_time) {
      if (inventory[productId] > 0) {
        inventory[productId]--;
        next_step_time =
            current_time +
            rng->uniform(
                min_interval,
                max_interval); // assuming processing time normal is 1.0
        return productId;
      } else {
        if (rng->bernoulli(supply_prob)) {
          int refill_amount = rng->uniformInt(min_prod_refill, max_prod_refill);
          inventory[productId] += refill_amount;
        }
        next_step_time =
            current_time +
            rng->uniform(
                min_interval,
                max_interval); // assuming processing time normal is 1.0

        return -productId;
      }
    } else {
      return 0;
    }
  }
  double getNextStepTime() { return next_step_time; }
};

// ============================================================================
// 18. SUPPLIER MODEL
// ============================================================================

// Supplier: Represents a supplier in a simulation.
// Purpose:
//  - Simulates the supply of products at specific intervals.
//  - Generates random supply requests periodically based on the defined
//  intervals.
// Usage:
//  Supplier supplier(minInterval, maxInterval, numProducts, rng);
//  int suppliedProduct = supplier.step(timeStep);
//  supplier.reset();
class Supplier {
private:
  double nextSupplyTime;
  double minInterval;
  double maxInterval;
  int numProducts;
  RandomGenerator &rng;

public:
  Supplier(double V, double Q, int N, RandomGenerator &generator)
      : minInterval(V), maxInterval(Q), numProducts(N), rng(generator) {
    nextSupplyTime = rng.uniform(V, Q);
  }

  int step(double dt) {
    nextSupplyTime -= dt;
    if (nextSupplyTime <= 0) {
      nextSupplyTime = rng.uniform(minInterval, maxInterval);
      return rng.uniformInt(1, numProducts);
    }
    return 0;
  }

  void reset() { nextSupplyTime = rng.uniform(minInterval, maxInterval); }
};

// Server di inventario con messaggi (customer/server network-style)
class InventoryServerProcess : public DiscreteEventProcess {
public:
  InventoryServerProcess(int pid, int numItems, int initialStockPerItem,
                         double serviceTime, MessageBus &bus,
                         RandomGenerator &rng)
      : pid_(pid), serviceTime_(serviceTime), bus_(bus), rng_(rng),
        nextEventTime_(0.0),
        storage_(static_cast<std::size_t>(numItems), initialStockPerItem),
        totalRequests_(0), missedRequests_(0) {}

  void initialize(double startTime) override {
    nextEventTime_ = startTime + serviceTime_;
  }

  double nextEventTime() const override { return nextEventTime_; }

  void handleEvent(double currentTime) override {
    if (currentTime < nextEventTime_) {
      return;
    }

    // processa TUTTI i messaggi arrivati a questo server
    auto &inbox = bus_.netToProc(pid_);
    while (!inbox.empty()) {
      Message m = inbox.front();
      inbox.pop();

      totalRequests_++;

      // sicurezza sugli indici prodotto
      if (m.item < 0 || static_cast<std::size_t>(m.item) >= storage_.size()) {
        continue; // scarta richieste invalide
      }

      double servedQty = 0.0;
      int &stock = storage_[static_cast<std::size_t>(m.item)];

      if (stock >= static_cast<int>(m.quantity)) {
        // richiesta soddisfatta
        stock -= static_cast<int>(m.quantity);
        servedQty = m.quantity;
      } else {
        // non abbastanza stock
        servedQty = static_cast<double>(stock);
        missedRequests_++;
        stock = 0;
      }

      // risposta al cliente
      Message reply;
      reply.time = currentTime;
      reply.sender = pid_;
      reply.receiver = m.sender; // rimanda al mittente
      reply.item = m.item;
      reply.quantity = servedQty;

      bus_.procToNet(pid_).push(reply);

      // eventuale logica di refill: esempio semplice
      if (stock == 0) {
        int refill = rng_.uniformInt(1, 10); // batch di rifornimento random
        stock += refill;
      }
    }

    nextEventTime_ = currentTime + serviceTime_;
  }

  const std::vector<int> &storage() const { return storage_; }

  int totalRequests() const { return totalRequests_; }
  int missedRequests() const { return missedRequests_; }

private:
  int pid_;
  double serviceTime_;
  MessageBus &bus_;
  RandomGenerator &rng_;

  double nextEventTime_;
  std::vector<int> storage_;

  int totalRequests_;
  int missedRequests_;
};

// ============================================================================
// 19. INVENTORY CUSTOMER PROCESS (DES-compatible)
// ============================================================================
//
// InventoryCustomerProcess:
//  - Processo a eventi discreti che rappresenta un cliente.
//  - Invia richieste al server tramite MessageBus (procToNet -> NetworkRouter).
//  - Legge le risposte dal canale netToProc.
//  - Statistiche: quante richieste ha mandato e quante risposte ha ricevuto.
//
class InventoryCustomerProcess : public DiscreteEventProcess {
public:
  // pid              : id di questo processo (indice nei canali del MessageBus)
  // serverPid        : pid del server a cui mandare le richieste
  // numItems         : numero di prodotti gestiti (item in [0, numItems-1])
  // minInterval/maxInterval : intervallo di tempo tra una richiesta e la
  // successiva
  InventoryCustomerProcess(int pid, int serverPid, int numItems,
                           double minInterval, double maxInterval,
                           MessageBus &bus, RandomGenerator &rng)
      : pid_(pid), serverPid_(serverPid), numItems_(numItems),
        minInterval_(minInterval), maxInterval_(maxInterval), bus_(bus),
        rng_(rng), nextEventTime_(0.0), issuedRequests_(0), receivedReplies_(0),
        totalRequestedQty_(0.0), totalServedQty_(0.0) {}

  // inizializza il prossimo evento (prima richiesta)
  void initialize(double startTime) override {
    nextEventTime_ = startTime + rng_.uniform(minInterval_, maxInterval_);
  }

  double nextEventTime() const override { return nextEventTime_; }

  void handleEvent(double currentTime) override {
    if (currentTime < nextEventTime_) {
      return;
    }

    // 1) genera una nuova richiesta al server
    const int item = rng_.uniformInt(
        0, numItems_ - 1); // coerente con InventoryServerProcess
    const int qty = rng_.uniformInt(1, 3); // quantità richiesta (1..3)

    Message req;
    req.time = currentTime;
    req.sender = pid_;
    req.receiver = serverPid_;
    req.item = item;
    req.quantity = static_cast<double>(qty);

    bus_.procToNet(pid_).push(req);
    issuedRequests_++;
    totalRequestedQty_ += req.quantity;

    // 2) processa tutte le risposte arrivate su questo cliente
    auto &inbox = bus_.netToProc(pid_);
    while (!inbox.empty()) {
      Message m = inbox.front();
      inbox.pop();

      // opzionale: possiamo filtrare solo messaggi dal serverPid_
      // if (m.sender != serverPid_) continue;

      receivedReplies_++;
      totalServedQty_ += m.quantity;
    }

    // 3) pianifica la prossima richiesta
    nextEventTime_ = currentTime + rng_.uniform(minInterval_, maxInterval_);
  }

  // --- metriche accessibili dall’esterno ---

  int pid() const { return pid_; }
  int serverPid() const { return serverPid_; }

  int issuedRequests() const { return issuedRequests_; }
  int receivedReplies() const { return receivedReplies_; }

  double totalRequestedQty() const { return totalRequestedQty_; }
  double totalServedQty() const { return totalServedQty_; }

  // fill-rate lato customer
  double serviceLevel() const {
    if (totalRequestedQty_ <= 0.0)
      return 1.0;
    return totalServedQty_ / totalRequestedQty_;
  }

private:
  int pid_;
  int serverPid_;
  int numItems_;
  double minInterval_;
  double maxInterval_;

  MessageBus &bus_;
  RandomGenerator &rng_;

  double nextEventTime_;

  int issuedRequests_;
  int receivedReplies_;
  double totalRequestedQty_;
  double totalServedQty_;
};

} // namespace SELib
