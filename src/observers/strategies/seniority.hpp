#ifndef SERENITY_STRATEGIES_DECIDER_SENIORITY_HPP
#define SERENITY_STRATEGIES_DECIDER_SENIORITY_HPP

#include "glog/logging.h"

#include "observers/strategies/base.hpp"

#include "serenity/config.hpp"
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
    // uint64_t
    // Specify the initial value of iterations we should wait until
    // we create new correction.
    this->fields[strategy::CONTENTION_COOLDOWN] =
      strategy::DEFAULT_CONTENTION_COOLDOWN;

    // double_t
    this->fields[strategy::STARTING_SEVERITY] =
      strategy::DEFAULT_STARTING_SEVERITY;
  }
};


/**
 * Checks contentions and choose executors to kill.
 * Currently it calculates mean contention and based on that estimates how
 * many executors we should kill. Executors are sorted by age.
 *
 * It also steers the valve filter using EventBus.
 */
class SeniorityStrategy : public RevocationStrategy {
 public:
  explicit SeniorityStrategy(const SerenityConfig& _config)
      : RevocationStrategy(Tag(QOS_CONTROLLER, "SeniorityStrategy")),
        cooldownCounter(None()),
        estimatorDisabled(false) {
    SerenityConfig config = SeniorityConfig(_config);
    this->cfgCooldownTime = config.getU64(strategy::CONTENTION_COOLDOWN);
    this->cfgDefaultSeverity = config.getD(strategy::STARTING_SEVERITY);
  }

  RevocationStrategyFunction decide;

  static const constexpr char* NAME = "SeniorityStrategy";
 private:
  bool estimatorDisabled;

  Option<uint64_t> cooldownCounter;

  // cfg parameters.
  uint64_t cfgCooldownTime;
  double_t cfgDefaultSeverity;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_STRATEGIES_DECIDER_SENIORITY_HPP
