#pragma once

#include <cmath>   // std::isfinite
#include <numeric> // std::accumulate
#include <random>
#include <stdexcept>
#include <vector>

namespace SELib {

class RandomGenerator {
private:
  std::mt19937 gen;

public:
  explicit RandomGenerator(unsigned seed = std::random_device{}())
      : gen(seed) {}

  double uniform(double min, double max) {
    auto lo = std::min(min, max);
    auto high = std::max(min, max);
    std::uniform_real_distribution<double> dis(lo, high);
    return dis(gen);
  }

  int uniformInt(int min, int max) {
    auto lo = std::min(min, max);
    auto high = std::max(min, max);
    std::uniform_int_distribution<int> dis(lo, high);
    return dis(gen);
  }

  double gaussian(double mean, double stddev) {
    if (stddev < 0.0) {
      throw std::invalid_argument("gaussian: stddev must be >= 0");
    }
    std::normal_distribution<double> dis(mean, stddev);
    return dis(gen);
  }

  bool bernoulli(double p) {
    if (p < 0.0 || p > 1.0 || !std::isfinite(p)) {
      throw std::invalid_argument("bernoulli: p must be in [0,1]");
    }
    std::bernoulli_distribution dis(p);
    return dis(gen);
  }

  int discrete(const std::vector<double> &probabilities) {
    if (probabilities.empty()) {
      throw std::invalid_argument("discrete: empty probability vector");
    }

    for (double w : probabilities) {
      if (w < 0.0 || !std::isfinite(w)) {
        throw std::invalid_argument(
            "discrete: weights must be non-negative and finite");
      }
    }

    double sum =
        std::accumulate(probabilities.begin(), probabilities.end(), 0.0);
    if (sum <= 0.0) {
      throw std::invalid_argument("discrete: sum of weights must be > 0");
    }

    std::discrete_distribution<int> dis(probabilities.begin(),
                                        probabilities.end());
    return dis(gen);
  }

  void setSeed(unsigned seed) { gen.seed(seed); }

  // (opzionale) accesso al motore se ti serve per altre distribuzioni
  std::mt19937 &engine() { return gen; }
};
} // namespace SELib
