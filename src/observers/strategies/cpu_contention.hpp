#ifndef SERENITY_STRATEGIES_CPU_CONTENTION_HPP
#define SERENITY_STRATEGIES_CPU_CONTENTION_HPP

#include "glog/logging.h"

#include "observers/strategies/base.hpp"

#include "serenity/config.hpp"
#include "serenity/data_utils.hpp"
#include "serenity/wid.hpp"

namespace mesos {
namespace serenity {

class CpuContentionStrategyConfig : public SerenityConfig {
 public:
  CpuContentionStrategyConfig() {
    this->initDefaults();
  }

  explicit CpuContentionStrategyConfig(const SerenityConfig& customCfg) {
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
    this->fields[strategy::DEFAULT_CPU_SEVERITY] =
      strategy::DEFAULT_DEFAULT_CPU_SEVERITY;
  }
};


/**
 * Checks contentions and choose executors to kill.
 * It accepts only Contention_Type_CPU.
 * Currently, it revokes firstly executors with utilization above their limits.
 * Then it sorts executors by age and get max contention severity.
 * Each severity means how many CPUs we should 'recover' from revocation.
 * It introduces cooldown and also steers the valve filter using EventBus.
 */
class CpuContentionStrategy : public RevocationStrategy {
 public:
  explicit CpuContentionStrategy(
      const SerenityConfig& _config,
      const lambda::function<usage::GetterFunction>& _cpuUsageGetFunction)
      : RevocationStrategy(Tag(QOS_CONTROLLER, "CpuContentionStrategy")),
        cpuUsageGetFunction(_cpuUsageGetFunction),
        cooldownCounter(None()),
        estimatorDisabled(false) {
    SerenityConfig config = CpuContentionStrategyConfig(_config);
    this->cfgCooldownTime = config.getU64(strategy::CONTENTION_COOLDOWN);
    this->cfgDefaultSeverity = config.getD(strategy::DEFAULT_CPU_SEVERITY);
  }

  RevocationStrategyFunction decide;

  static const constexpr char* NAME = "CpuContentionStrategy";

 private:
  bool estimatorDisabled;
  Option<uint64_t> cooldownCounter;
  const lambda::function<usage::GetterFunction> cpuUsageGetFunction;

  // cfg parameters.
  uint64_t cfgCooldownTime;
  double_t cfgDefaultSeverity;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_STRATEGIES_CPU_CONTENTION_HPP
