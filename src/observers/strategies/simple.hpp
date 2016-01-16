#ifndef SERENITY_STRATEGIES_DECIDER_SIMPLE_HPP
#define SERENITY_STRATEGIES_DECIDER_SIMPLE_HPP

#include "glog/logging.h"

#include "observers/strategies/base.hpp"

#include "serenity/config.hpp"
#include "serenity/wid.hpp"

namespace mesos {
namespace serenity {

class SimpleStrategyConfig : public SerenityConfig {
 public:
  SimpleStrategyConfig() {
    this->initDefaults();
  }

  explicit SimpleStrategyConfig(const SerenityConfig& customCfg) {
    this->initDefaults();
    this->applyConfig(customCfg);
  }

  void initDefaults() {
  }
};


/**
 * TBD
 */
class SimpleStrategy : public RevocationStrategy {
 public:
  explicit SimpleStrategy(const SerenityConfig& _config = SerenityConfig())
      : RevocationStrategy(Tag(QOS_CONTROLLER, "SimpleStrategy")),
        cooldownCounter(None()),
        estimatorDisabled(false) {
    SerenityConfig config = SimpleStrategyConfig(_config);
  }

  RevocationStrategyFunction decide;

  static const constexpr char* NAME = "SimpleStrategy";
 private:
  bool estimatorDisabled;

  Option<uint64_t> cooldownCounter;

  // cfg parameters.
  uint64_t cfgCooldownTime;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_STRATEGIES_DECIDER_SIMPLE_HPP
