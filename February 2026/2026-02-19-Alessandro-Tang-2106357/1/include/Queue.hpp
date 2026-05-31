#pragma once

#include <cstddef>
#include <cstdint>
#include <queue>
#include <stdexcept>

namespace SELib {
// ============================================================================
// 9. FIFO QUEUE
// ============================================================================

// FIFOQueue: Implements a first-in-first-out (FIFO) queue with a fixed maximum
// size. Purpose:
//  - Enqueue and dequeue items in FIFO order.
//  - Track statistics such as total items enqueued and dequeued.
//  - Support dynamic size constraints.
// Usage:
//  FIFOQueue<int> queue(10);
//  queue.enqueue(5);
//  int item;
//  if (queue.dequeue(item)) {
//    // Process item.
//  }
template <typename T> class FIFOQueue {
private:
  std::queue<T> queue;
  size_t maxSize;
  size_t totalEnqueued;
  size_t totalDequeued;

public:
  FIFOQueue(size_t max = SIZE_MAX)
      : maxSize(max), totalEnqueued(0), totalDequeued(0) {}

  bool enqueue(const T &item) {
    if (queue.size() >= maxSize)
      return false;
    queue.push(item);
    totalEnqueued++;
    return true;
  }

  bool dequeue(T &item) {
    if (queue.empty())
      return false;
    item = queue.front();
    queue.pop();
    totalDequeued++;
    return true;
  }

  bool isEmpty() const { return queue.empty(); }
  bool isFull() const { return queue.size() >= maxSize; }
  size_t size() const { return queue.size(); }
  size_t getTotalEnqueued() const { return totalEnqueued; }
  size_t getTotalDequeued() const { return totalDequeued; }
  std::queue<T> getQueue() const { return queue; }
  T getFirst() const {
    if (queue.empty())
      throw std::runtime_error("Queue is empty");
    return queue.front();
  }

  void clear() {
    while (!queue.empty())
      queue.pop();
  }
};
} // namespace SELib
