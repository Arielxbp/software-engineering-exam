#pragma once

#include "Random.hpp"
#include "Stat.hpp"
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <stdexcept>
#include <vector>

namespace SELib {

// ============================================================================
// 7. MARKOV CHAIN
// ============================================================================

// MarkovChain: Implements a discrete-time Markov chain.
// Purpose:
//  - Simulates stochastic processes with defined states and transition
//  probabilities.
//  - Useful in modeling random systems and computing related probabilities.
// Usage:
//  MarkovChain mc(3, rng);
//  mc.setTransition(0, 1, 0.5);
//  mc.setTransition(0, 2, 0.5);
//  mc.setState(0);
//  int nextState = mc.step();
class MarkovChain {
private:
  int numStates;
  std::vector<std::vector<double>> transitionMatrix;
  int currentState;
  RandomGenerator &rng;

public:
  MarkovChain(int states, RandomGenerator &generator)
      : numStates(states), currentState(0), rng(generator) {
    transitionMatrix.resize(states, std::vector<double>(states, 0.0));
  }

  void setTransition(int from, int to, double probability) {
    if (from >= 0 && from < numStates && to >= 0 && to < numStates) {
      transitionMatrix[from][to] = probability;
    }
  }

  void setState(int state) {
    if (state >= 0 && state < numStates) {
      currentState = state;
    }
  }

  int getState() const { return currentState; }

  int step() {
    currentState = rng.discrete(transitionMatrix[currentState]);
    return currentState;
  }

  double getTransitionProb(int from, int to) const {
    return transitionMatrix[from][to];
  }

  double expectedSojournTime(int state, double timeStep = 1.0) const {
    double pii = transitionMatrix[state][state];
    if (pii >= 1.0)
      return std::numeric_limits<double>::infinity();
    return timeStep / (1.0 - pii);
  }
};

// ============================================================================
// 8. MDP
// ============================================================================

// MDP: Implements a Markov Decision Process (MDP) model.
// Purpose:
//  - Simulate decision-making processes with states, transitions, and
//  associated costs.
//  - Calculate the total cost to reach a target state from a starting state.
// Usage:
//  - Add transitions between states with associated probabilities and costs
//  using `addTransition`.
//  - Use `step` to simulate a single transition and calculate its cost.
//  - Use `simulate` to estimate the total cost from a start to an end state.
class MDP {
private:
  int numStates;
  std::map<int, std::vector<std::pair<int, std::pair<double, double>>>>
      transitions;
  RandomGenerator &rng;

public:
  MDP(int states, RandomGenerator &generator)
      : numStates(states), rng(generator) {}

  void addTransition(int from, int to, double probability, double cost) {
    transitions[from].push_back({to, {probability, cost}});
  }

  std::pair<int, double> step(int currentState) {
    if (transitions.find(currentState) == transitions.end()) {
      return {currentState, 0.0};
    }

    std::vector<double> probs;
    for (const auto &t : transitions[currentState]) {
      probs.push_back(t.second.first);
    }

    int idx = rng.discrete(probs);
    int nextState = transitions[currentState][idx].first;
    double cost = transitions[currentState][idx].second.second;

    return {nextState, cost};
  }

  double simulate(int startState, int endState) {
    int state = startState;
    double totalCost = 0.0;
    int maxSteps = 100000;

    for (int step = 0; step < maxSteps && state != endState; step++) {
      auto [nextState, cost] = this->step(state);
      totalCost += cost;
      state = nextState;
    }
    return totalCost;
  }
};

// ============================================================================
// 10. MONTE CARLO SIMULATOR
// ============================================================================

// MonteCarloSimulator: A utility for running Monte Carlo simulations.
// Purpose:
//  - Estimate statistical metrics through repeated random sampling.
//  - Provides methods to calculate mean results and probabilities of events.
// Usage:
//  MonteCarloSimulator simulator(1000);
//  double estimate = simulator.estimate([](int) { return some_random_value; });
//  double probability = simulator.estimateProbability([](int) { return
//  some_event_occurs; });
class MonteCarloSimulator {
private:
  Statistics stats;
  int numSimulations;

public:
  MonteCarloSimulator(int simulations = 1000) : numSimulations(simulations) {}

  template <typename SimFunc> double estimate(SimFunc simulation) {
    stats.clear();
    for (int i = 0; i < numSimulations; i++) {
      double result = simulation(i);
      stats.addSample(result);
    }
    return stats.mean();
  }

  template <typename SimFunc> double estimateProbability(SimFunc simulation) {
    int successes = 0;
    for (int i = 0; i < numSimulations; i++) {
      if (simulation(i))
        successes++;
    }
    return static_cast<double>(successes) / numSimulations;
  }

  const Statistics &getStatistics() const { return stats; }
};

// ============================================================================
// 13. OPTIMIZATION UTILITIES
// ============================================================================

// Optimizer: Utility class offering optimization techniques such as random
// search and grid search. Purpose:
//  - Explore parameter spaces to maximize or minimize an objective function.
//  - Supports both continuous and discrete search spaces.
// Usage:
//  - Use `randomSearch` for stochastic optimization over a defined range.
//  - Use `gridSearch` for systematic exploration in evenly spaced steps.
// Example:
//  auto [bestParam, bestValue] = Optimizer::randomSearch(objectiveFunction,
//  0.0, 10.0, 100, rng); auto [bestInt, bestIntValue] =
//  Optimizer::randomSearchInt(objectiveFunction, 0, 10, 50, rng);
class Optimizer {
public:
  template <typename ObjFunc>
  static std::pair<double, double>
  randomSearch(ObjFunc objective, double minValue, double maxValue, int budget,
               RandomGenerator &rng, bool maximize = true) {
    double bestParam = minValue;
    double bestValue = maximize ? -std::numeric_limits<double>::infinity()
                                : std::numeric_limits<double>::infinity();

    for (int i = 0; i < budget; i++) {
      double param = rng.uniform(minValue, maxValue);
      double value = objective(param);

      if ((maximize && value > bestValue) || (!maximize && value < bestValue)) {
        bestValue = value;
        bestParam = param;
      }
    }
    return {bestParam, bestValue};
  }

  template <typename ObjFunc>
  static std::pair<int, double>
  randomSearchInt(ObjFunc objective, int minValue, int maxValue, int budget,
                  RandomGenerator &rng, bool maximize = true) {
    int bestParam = minValue;
    double bestValue = maximize ? -std::numeric_limits<double>::infinity()
                                : std::numeric_limits<double>::infinity();

    if (budget <= 0) {
      throw std::invalid_argument("randomSearchInt: budget must be positive.");
    }

    if (minValue >= maxValue) {
      throw std::invalid_argument(
          "randomSearchInt: minValue must be less than maxValue.");
    }

    for (int i = 0; i < budget; i++) {
      int param = rng.uniformInt(minValue, maxValue);
      double value = objective(param);

      if ((maximize && value > bestValue) || (!maximize && value < bestValue)) {
        bestValue = value;
        bestParam = param;
      }
    }
    return {bestParam, bestValue};
  }

  template <typename ObjFunc>
  static std::pair<double, double> gridSearch(ObjFunc objective,
                                              double minValue, double maxValue,
                                              int steps, bool maximize = true) {
    double bestParam = minValue;
    double bestValue = maximize ? -std::numeric_limits<double>::infinity()
                                : std::numeric_limits<double>::infinity();
    double stepSize = (maxValue - minValue) / steps;

    if (steps <= 0) {
      throw std::invalid_argument("gridSearch: steps must be positive.");
    }

    for (int i = 0; i <= steps; i++) {
      double param = minValue + i * stepSize;
      double value = objective(param);

      if ((maximize && value > bestValue) || (!maximize && value < bestValue)) {
        bestValue = value;
        bestParam = param;
      }
    }
    return {bestParam, bestValue};
  }
};
} // namespace SELib
