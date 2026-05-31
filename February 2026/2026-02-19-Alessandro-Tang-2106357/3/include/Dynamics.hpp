#pragma once

#include <functional>
#include <vector>
namespace SELib {
// ============================================================================
// 3. ODE SOLVER (Runge-Kutta 4th order)
// ============================================================================

using StateVector = std::vector<double>;
using DerivativeFunction = std::function<StateVector(
    double, const StateVector &, const StateVector &)>;

// ODESolver: Implements numerical methods for solving Ordinary Differential
// Equations (ODEs). Purpose:
//  - Provides tools for simulating dynamic systems modeled by ODEs.
//  - Includes methods such as Runge-Kutta 4th order and Euler's method for
//  integration.
// Usage:
//  - Pass a derivative function that defines the ODE system.
//  - Use `rk4Step` for higher accuracy or `eulerStep` for simpler systems.
// Example:
//  StateVector nextState = ODESolver::rk4Step(0.0, currentState, controlInputs,
//  dt, dynamicsFunction);
class ODESolver {
public:
  static StateVector rk4Step(double t, const StateVector &x,
                             const StateVector &u, double dt,
                             DerivativeFunction f) {
    StateVector k1 = f(t, x, u);
    StateVector x2 = addScaled(x, k1, dt / 2.0);
    StateVector k2 = f(t + dt / 2.0, x2, u);
    StateVector x3 = addScaled(x, k2, dt / 2.0);
    StateVector k3 = f(t + dt / 2.0, x3, u);
    StateVector x4 = addScaled(x, k3, dt);
    StateVector k4 = f(t + dt, x4, u);

    StateVector xNew = x;
    for (size_t i = 0; i < x.size(); i++) {
      xNew[i] += (dt / 6.0) * (k1[i] + 2.0 * k2[i] + 2.0 * k3[i] + k4[i]);
    }
    return xNew;
  }

  static StateVector eulerStep(double t, const StateVector &x,
                               const StateVector &u, double dt,
                               DerivativeFunction f) {
    StateVector dx = f(t, x, u);
    StateVector xNew = x;
    for (size_t i = 0; i < x.size(); i++) {
      xNew[i] += dx[i] * dt;
    }
    return xNew;
  }

private:
  static StateVector addScaled(const StateVector &x, const StateVector &dx,
                               double scale) {
    StateVector result = x;
    for (size_t i = 0; i < x.size(); i++) {
      result[i] += dx[i] * scale;
    }
    return result;
  }
};

// ============================================================================
// 4. PID CONTROLLER
// ============================================================================

// PIDController: Implements a Proportional-Integral-Derivative (PID)
// controller. Purpose:
//  - Automatically adjusts a control signal to minimize the difference between
//  a desired setpoint and a measured value.
//  - Used in control systems to regulate processes like temperature, speed, or
//  position.
// Usage:
//  - Create an instance with proportional (Kp), integral (Ki), and derivative
//  (Kd) gains, along with a time step.
//  - Use `compute` to calculate the control signal based on the setpoint and
//  measurement.
// Example:
//  PIDController pid(1.0, 0.1, 0.01, 0.1);
//  double controlSignal = pid.compute(desiredValue, actualValue);
class PIDController {
private:
  double kp, ki, kd;
  double integral;
  double prevError;
  double dt;

public:
  PIDController(double Kp, double Ki, double Kd, double timeStep)
      : kp(Kp), ki(Ki), kd(Kd), integral(0), prevError(0), dt(timeStep) {}

  double compute(double setpoint, double measurement) {
    double error = setpoint - measurement;
    integral += error * dt;
    double derivative = (error - prevError) / dt;
    prevError = error;
    return kp * error + ki * integral + kd * derivative;
  }

  void reset() {
    integral = 0;
    prevError = 0;
  }

  void setGains(double Kp, double Ki, double Kd) {
    kp = Kp;
    ki = Ki;
    kd = Kd;
  }
};

// ============================================================================
// 5. CONTINUOUS TIME SIMULATION
// ============================================================================

// ContinuousTimeSimulation: A base class for simulating continuous-time dynamic
// systems. Purpose:
//  - Provides a framework for simulating systems characterized by continuous
//  dynamics over time.
//  - Integrates system dynamics using a Runge-Kutta 4th order solver.
// Usage:
//  - Derive from this class and implement the `dynamics`, `computeControl`,
//  `initialize`, and `finalize` methods.
//  - Customize the `onStep` method for per-step side effects, if required.
// Example:
//  class MySimulation : public ContinuousTimeSimulation {
//    // Implement the required virtual methods.
//  };
//  MySimulation sim(dt, horizon, initialState);
//  sim.run();
class ContinuousTimeSimulation {
protected:
  double currentTime;
  double timeStep;
  double horizon;
  StateVector state;
  StateVector control;

public:
  ContinuousTimeSimulation(double dt, double H, const StateVector &initialState)
      : currentTime(0.0), timeStep(dt), horizon(H), state(initialState) {
    control.resize(initialState.size(), 0.0);
  }

  virtual ~ContinuousTimeSimulation() = default;

  virtual StateVector dynamics(double t, const StateVector &x,
                               const StateVector &u) = 0;
  virtual StateVector computeControl(double t, const StateVector &x) = 0;
  virtual void initialize() = 0;
  virtual void onStep() {}
  virtual void finalize() = 0;

  void run() {
    initialize();
    while (currentTime < horizon) {
      control = computeControl(currentTime, state);
      state = ODESolver::rk4Step(
          currentTime, state, control, timeStep,
          [this](double t, const StateVector &x, const StateVector &u) {
            return this->dynamics(t, x, u);
          });
      currentTime += timeStep;
      onStep();
    }
    finalize();
  }

  double getCurrentTime() const { return currentTime; }
  const StateVector &getState() const { return state; }
  const StateVector &getControl() const { return control; }
};

// ============================================================================
// 6. SAMPLE AND HOLD
// ============================================================================

// SampleAndHold: A utility to store a value and update it periodically.
// Purpose:
//  - Maintain a value that gets updated periodically based on a time interval.
//  - Useful for sampling data and ensuring updates happen at fixed intervals.
// Usage:
//  SampleAndHold<int> sampler(5.0, 0);
//  if (sampler.shouldUpdate(currentTime)) {
//    sampler.update(currentTime, newValue);
//  }
//  int currentValue = sampler.getValue();
template <typename T> class SampleAndHold {
private:
  T value;
  double lastUpdateTime;
  double samplePeriod;

public:
  SampleAndHold(double period, const T &initialValue = T())
      : value(initialValue), lastUpdateTime(0), samplePeriod(period) {}

  bool shouldUpdate(double currentTime) const {
    return (currentTime - lastUpdateTime) >= samplePeriod;
  }

  void update(double currentTime, const T &newValue) {
    value = newValue;
    lastUpdateTime = currentTime;
  }

  const T &getValue() const { return value; }
  void reset() { lastUpdateTime = 0; }
};
} // namespace SELib
