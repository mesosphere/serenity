#ifndef SERENITY_TESTS_LOAD_GENERATOR_HPP
#define SERENITY_TESTS_LOAD_GENERATOR_HPP

#include <cmath>

#include "mesos/mesos.pb.h"

#include "stout/lambda.hpp"

namespace mesos {
namespace serenity {
namespace tests {

constexpr double_t DEFAULT_TIME_WINDOW = 1;
constexpr double_t DEFAULT_START_TIMESTAMP = 34223425;
constexpr double_t DEFAULT_MAX_NOISE = 50;

//! Math Functions - used for load model.
namespace math {

inline double_t linearFunction(double_t x) {
  return x;
}


inline double_t sinFunction(double_t x) {
  return sin(x) + cos(x);
}

}  // namespace math


/**
 * Base class for all Noise Generators.
 * It is needed for Load Generator to introduce noise in samples.
 */
class NoiseGenerator {
 public:
  virtual double_t generate(int32_t iteration) = 0;
};


/**
 * Primary Noise Generator - generates no noise.
 */
class ZeroNoise : public NoiseGenerator {
 public:
  double_t generate(int32_t iteration) {
    return 0;
  }
};


/**
 * Symetric Noise Generator produces deterministic noise.
 * It is similar to sine wave. It alternates: raises to maxNoise
 * and -maxNoise, then stays low.
 * Average of generated values equals 0.
 */
class SymetricNoiseGenerator : public NoiseGenerator {
 public:
  explicit SymetricNoiseGenerator(double_t _maxNoise) : maxNoise(_maxNoise) {}

  double_t generate(int32_t iteration) {
    sign *= -1;
    if (iteration % 2 == 0) {
      noise += noiseModifier;
      if (std::abs(noise) >= maxNoise) noiseModifier *= -1;
    }
    return (noise * sign);
  }

  double_t noiseModifier = 2;
  double_t maxNoise = DEFAULT_MAX_NOISE;

 private:
  int32_t sign = -1;
  double_t noise = 0;
};


/**
 * Sample primitive.
 */
struct LoadSample {
  LoadSample() {}

  explicit LoadSample(
      double_t _value, double_t _noise, double_t _timestamp)
      : value(_value), noise(_noise), timestamp(_timestamp) {}

  // Get a sample with noise introduced.
  double_t operator()() const {
    return value + noise;
  }

  // Get a sample without any noise.
  double_t clearValue() const {
    return value;
  }

  // Print Sample in CSV format.
  // Columns: Value; Noise+Value; Result \n
  void printCSVLine(double_t result) const {
    std::cout
        << value << "; "
        << this->operator()() << "; "
        << result << std::endl;
  }

  double_t value;
  double_t noise;
  double_t timestamp;
};


/**
 * Main class for Load Generator.
 * It generates samples for each loop iteration (increment function).
 * Features:
 * - Generated load is modeled via input function.
 * - Noise can be introduced via Noise Generators.
 * - Stop condition after iterations max exceeded.
 * - Optionally you can start iteration from defied value.
 * - Optionally you can make complicated scenario by modifying load via
 *    public modifier field.
 */
class LoadGenerator {
 public:
  explicit LoadGenerator(
      const lambda::function<double_t(double_t)>& _modelFunction,
      NoiseGenerator* _noiseGen,
      const int32_t _iterations)
    : LoadGenerator(_modelFunction, _noiseGen, 0, _iterations) {}

  explicit LoadGenerator(
      const lambda::function<double_t(double_t)>& _modelFunction,
      NoiseGenerator* _noiseGen,
      const int32_t _iteration,
      const int32_t _iterations)
      : modelFunction(_modelFunction),
        noiseGen(_noiseGen),
        iteration(_iteration),
        iterations(_iterations),
        done(false),
        i(_modelFunction(_iteration), 0, DEFAULT_START_TIMESTAMP) {}

  ~LoadGenerator() {}

  typedef LoadSample const& reference;
  typedef LoadSample const* pointer;

  bool end() {
    return !done;
  }

  reference operator*() const { return i; }
  pointer operator->() const { return &i; }

  LoadGenerator& operator++() {
    if (done) return *this;

    iteration++;
    if (iteration >= iterations) {
      done = true;
      return *this;
    }

    i.value = modifier + modelFunction(iteration);
    i.timestamp += timeWindow;
    i.noise = noiseGen->generate(iteration);

    if (dbg) std::cout << iteration << std::endl;
    return *this;
  }

  LoadGenerator operator++(int32_t) {
    LoadGenerator const tmp(*this);
    ++*this;
    return tmp;
  }

  double_t modifier = 0;
  int32_t iteration;
  bool dbg = false;

 protected:
  const lambda::function<double_t(double_t)>& modelFunction;
  NoiseGenerator* noiseGen;
  const int32_t iterations;
  bool done;
  LoadSample i;

  double_t timeWindow = DEFAULT_TIME_WINDOW;
};


/**
 * This function fills instructions and cycles perf events to match given
 * IPC value.
 */
inline ResourceUsage_Executor generateIPC(
    const ResourceUsage_Executor _executorUsage,
    double_t _IpcValue,
    double_t _timestamp) {
  const int ACCURACY_MODIFIER = 10;
  // This function generates IPC with 1/ACCURACY_MODIFIER accuracy.
  _IpcValue *= ACCURACY_MODIFIER;
  ResourceUsage_Executor executorUsage;
  executorUsage.CopyFrom(_executorUsage);

  double_t previousInstructions =
    executorUsage.statistics().perf().instructions();
  double_t previousCycles =
    executorUsage.statistics().perf().cycles();

  executorUsage.mutable_statistics()
      ->mutable_perf()->set_instructions(previousInstructions + _IpcValue);

  executorUsage.mutable_statistics()
      ->mutable_perf()->set_cycles(previousCycles + ACCURACY_MODIFIER);

  executorUsage.mutable_statistics()
      ->mutable_perf()->set_timestamp(_timestamp);

  return executorUsage;
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_TESTS_LOAD_GENERATOR_HPP
