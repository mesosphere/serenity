#ifndef PWAVE_SCENARIO_HPP
#define PWAVE_SCENARIO_HPP

#include <iostream>
#include <map>

#include "generator.hpp"

namespace pwave {

class SignalScenario : public SignalGenerator {
 public:
  explicit SignalScenario(const size_t _iterations) :
    prepareIteraion(0), firstIter(true), SignalGenerator(_iterations) {}


  SignalScenario& after(const size_t iterations) {
    this->prepareIteraion += iterations;

    return *this;
  }

  SignalScenario& add(const double_t value) {
    this->addAt(this->prepareIteraion, value);

    return *this;
  }

  SignalScenario& addAt(const size_t iterations, const double_t value) {
    auto signalModifier = this->signalModifiers.find(this->iteration);
    if (signalModifier == this->signalModifiers.end()) {
      this->signalModifiers[iterations] = 0;
    }

    // Modifiers are cumulative.
    this->signalModifiers[iterations] += value;

    return *this;
  }

  SignalScenario& constantAdd(const double_t value, const size_t duration) {
    this->constantAddAt(this->prepareIteraion, value, duration);

    return *this;
  }

  SignalScenario& constantAddAt(
      const size_t iterations, const double_t value, const size_t duration) {
    for (size_t interIter = 0; interIter < duration; interIter++) {
      this->addAt(iterations + interIter, value);
    }
    return *this;
  }

  SignalScenario& use(lambda::function<double_t(double_t)> model) {
    this->useAt(this->prepareIteraion, model);

    return *this;
  }

  SignalScenario& useAt(const size_t iterations,
                        lambda::function<double_t(double_t)> model) {
    this->modelModifiers[iterations] = model;

    return *this;
  }

  SignalScenario& use(NoiseGenerator* noise) {
    this->useAt(this->prepareIteraion, noise);

    return *this;
  }

  SignalScenario& useAt(const size_t iterations,
                        NoiseGenerator* noise) {
    this->noiseModifiers[iterations] = std::shared_ptr<NoiseGenerator>(noise);

    return *this;
  }

  SignalScenario& operator++() {
    this->applyModifiers();
    // Run base function.
    SignalGenerator::operator++();

    return *this;
  }

  SignalScenario operator++(int32_t) {
    SignalScenario const tmp(*this);
    ++(*this);
    return tmp;
  }

  reference operator*() {
    if (firstIter) {
      applyModifiers(0);
      i = SignalSample(modelFunction(iteration), 0, DEFAULT_START_TIMESTAMP);
      firstIter = false;
    }
    return i;
  }
  pointer operator->() {
    if (firstIter) {
      applyModifiers(0);
      i = SignalSample(modelFunction(iteration), 0, DEFAULT_START_TIMESTAMP);
      firstIter = false;
    }
    return &i;
  }

 protected:
  bool firstIter;
  size_t prepareIteraion;
  std::map<size_t, lambda::function<double_t(double_t)>> modelModifiers;
  std::map<size_t, std::shared_ptr<NoiseGenerator>> noiseModifiers;
  std::map<size_t, double_t> signalModifiers;

  // Applies modifiers as default for NEXT iteration.
  void applyModifiers(int futureIter = 1) {
    auto modelModifier =
      this->modelModifiers.find(this->iteration + futureIter);
    if (modelModifier != this->modelModifiers.end()) {
      // Apply modelModifier.
      this->modelFunction = modelModifier->second;
    }

    auto noiseModifer =
      this->noiseModifiers.find(this->iteration + futureIter);
    if (noiseModifer != this->noiseModifiers.end()) {
      // Apply noiseModifer.
      this->noiseGen = noiseModifer->second;
    }

    auto signalModifier =
      this->signalModifiers.find(this->iteration + futureIter);
    if (signalModifier != this->signalModifiers.end()) {
      // Apply signalModifier.
      this->modifier += signalModifier->second;
    }
  }
};


}  // namespace pwave

#endif  // PWAVE_SCENARIO_HPP
