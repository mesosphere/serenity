#ifndef PWAVE_GENERATOR_HPP
#define PWAVE_GENERATOR_HPP

#include <cmath>
#include <iostream>
#include <memory>

#include "stout/lambda.hpp"

#include "noise.hpp"

namespace pwave {

#define ITERATE_SIGNAL(generator) for (; generator.end(); generator++)

constexpr double_t DEFAULT_TIME_WINDOW = 1;
constexpr double_t DEFAULT_START_TIMESTAMP = 34223425;


//! Math Functions - used for load model.
namespace math {

inline double_t const10Function(double_t x) {
  return 10;
}

inline double_t linearFunction(double_t x) {
  return x;
}

inline double_t sinFunction(double_t x) {
  return sin(x) + cos(x);
}

}  // namespace math


/**
 * Sample primitive.
 */
struct SignalSample {
  SignalSample() {}

  explicit SignalSample(
      double_t _value,
      double_t _noise,
      double_t _timestamp,
      double_t _cumulativeValue = 0.0)
    : value(_value),
      noise(_noise),
      timestamp(_timestamp),
      cumulativeValue(_cumulativeValue) {}

  // Get a sample with noise introduced.
  double_t operator()() const {
    return value + noise;
  }

  // Get a sample without any noise.
  double_t clearValue() const {
    return value;
  }

  // Get a cumulative sample with noise.
  double_t cumulative() const {
    return this->operator()() + cumulativeValue;
  }

  // Print Sample in CSV format.
  // Columns: Value; Noise+Value; Result \n
  void printCSVLine(double_t result) const {
    std::cout << value << "; "
        << this->operator()() << "; "
        << result << std::endl;
  }

  double_t value;
  double_t noise;
  double_t timestamp;
  double_t cumulativeValue;
};


/**
 * Main class for Signal Generator.
 * It generates samples for each loop iteration (increment function).
 * Features:
 * - Generated signal is modeled via input function.
 * - Noise can be introduced via Noise Generators.
 * - Stop condition after iterations max exceeded.
 * - Optionally you can start iteration from defied value.
 * - Optionally you can make complicated scenario by modifying signal via
 *    public modifier field.
 */
class SignalGenerator {
 public:
  explicit SignalGenerator(
      const size_t _iterations,
      lambda::function<double_t(double_t)> _modelFunction =
        math::const10Function,
      std::shared_ptr<NoiseGenerator> _noiseGen =
        std::shared_ptr<NoiseGenerator>(new ZeroNoise()))
    : SignalGenerator(0, _iterations, _modelFunction, _noiseGen) {}

  explicit SignalGenerator(
      const size_t _iteration,
      const size_t _iterations,
      lambda::function<double_t(double_t)>& _modelFunction,
      std::shared_ptr<NoiseGenerator> _noiseGen)
      : iteration(_iteration),
        iterations(_iterations),
        modelFunction(_modelFunction),
        noiseGen(_noiseGen),
        done(false),
        i(_modelFunction(_iteration), 0, DEFAULT_START_TIMESTAMP) {}

  ~SignalGenerator() {}

  typedef SignalSample const& reference;
  typedef SignalSample const* pointer;

  bool end() {
    return !done;
  }

  virtual reference operator*() const { return i; }
  virtual pointer operator->() const { return &i; }

  // Main signal generation logic.
  SignalGenerator& operator++() {
    if (done) return *this;

    iteration++;
    // Stop condition.
    if (iteration >= iterations) {
      done = true;
      return *this;
    }

    // Cumulate previous value.
    i.cumulativeValue += i.operator()();

    // Applying modelFunction.
    i.value = modifier + modelFunction(iteration);
    i.timestamp += timeWindow;
    // Apply optional noise.
    i.noise = noiseGen->generate(iteration);

    if (dbg) std::cout << iteration << std::endl;
    return *this;
  }

  SignalGenerator operator++(int32_t) {
    SignalGenerator const tmp(*this);
    ++*this;
    return tmp;
  }

  double_t modifier = 0;
  size_t iteration;
  bool dbg = false;

 protected:
  lambda::function<double_t(double_t)> modelFunction;
  std::shared_ptr<NoiseGenerator> noiseGen;
  const size_t iterations;
  bool done;
  SignalSample i;

  double_t timeWindow = DEFAULT_TIME_WINDOW;
};

}  // namespace pwave

#endif  // PWAVE_GENERATOR_HPP
