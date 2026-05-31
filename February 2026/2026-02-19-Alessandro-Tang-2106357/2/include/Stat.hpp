#pragma once

#include <cmath>
#include <functional>
#include <vector>

namespace SELib {
// ============================================================================
// 2. STATISTICS
// ============================================================================

// Statistics: A utility class for managing and analyzing numerical data
// samples. Purpose:
//  - Compute statistical metrics such as mean and standard deviation.
//  - Estimate probabilities of custom conditions.
//  - Check convergence of data over iterations.
// Usage:
//  Statistics stats;
//  stats.addSample(5.0);
//  double meanValue = stats.mean();
class Statistics {
private:
  std::vector<double> samples;

public:
  void addSample(double value) { samples.push_back(value); }

  void clear() { samples.clear(); }

  double mean() const {
    if (samples.empty())
      return 0.0;
    double sum = 0.0;
    for (double s : samples)
      sum += s;
    return sum / samples.size();
  }

  double stddev() const {
    if (samples.size() < 2)
      return 0.0;
    double m = mean();
    double variance = 0.0;
    for (double s : samples) {
      variance += (s - m) * (s - m);
    }
    return std::sqrt(variance / (samples.size() - 1));
  }

  double probability(std::function<bool(double)> condition) const {
    if (samples.empty())
      return 0.0;
    int count = 0;
    for (double s : samples) {
      if (condition(s))
        count++;
    }
    return static_cast<double>(count) / samples.size();
  }

  size_t size() const { return samples.size(); }
  const std::vector<double> &getSamples() const { return samples; }

  bool hasConverged(double tolerance = 0.1) const {
    if (samples.size() < 30)
      return false;
    double m = mean();
    double sd = stddev();
    return sd <= tolerance * std::abs(m);
  }
};
} // namespace SELib
