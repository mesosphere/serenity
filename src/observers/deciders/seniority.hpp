#ifndef SERENITY_CONTENTION_DECIDER_SENIORITY_HPP
#define SERENITY_CONTENTION_DECIDER_SENIORITY_HPP

#include "glog/logging.h"

#include "observers/deciders/base.hpp"

#include "serenity/wid.hpp"

namespace mesos {
namespace serenity {

class SeniorityConfig : public SerenityConfig {
 public:
  SeniorityConfig() {
    this->initDefaults();
  }

  explicit SeniorityConfig(const SerenityConfig& customCfg) {
    this->initDefaults();
    this->applyConfig(customCfg);
  }

  void initDefaults() {
    // Specify the initial value of iterations we should wait until
    // we create new correction.
    this->fields[decider::CONTENTION_COOLDOWN] =
      decider::DEFAULT_CONTENTION_COOLDOWN;
  }
};


/**
 * Checks contentions and choose executors to kill.
 * Currently it calculates mean contention and based on that estimates how
 * many executors we should kill. Executors are sorted by age.
 */
class SeniorityDecider : public ContentionDecider {
 public:
  explicit SeniorityDecider(const SerenityConfig& _config)
      : config(SeniorityConfig(_config))  {}

  ContentionDeciderFunction decide;

 private:
  const SerenityConfig config;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_CONTENTION_DECIDER_SENIORITY_HPP
