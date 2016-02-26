#ifndef SERENITY_STRATEGIES_CPU_CONTENTION_HPP
#define SERENITY_STRATEGIES_CPU_CONTENTION_HPP

#include "glog/logging.h"

#include "observers/strategies/base.hpp"

#include "serenity/config.hpp"
#include "serenity/data_utils.hpp"
#include "serenity/wid.hpp"

namespace mesos {
namespace serenity {

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
      const Config& _config,
      const lambda::function<usage::GetterFunction>& _cpuUsageGetFunction)
      : RevocationStrategy(Tag(QOS_CONTROLLER, "CpuContentionStrategy")),
        getCpuUsage(_cpuUsageGetFunction) {
    setDefaultSeverity(_config.getValue<double_t>(DEFAULT_CPU_SEVERITY_KEY));
  }

  Try<QoSCorrections> decide(ExecutorAgeFilter* ageFilter,
                             const Contentions& currentContentions,
                             const ResourceUsage& currentUsage);

  void setDefaultSeverity(const Result<double_t> value) {
    cfgDefaultSeverity =
      ConfigValidator<double_t>(value, DEFAULT_CPU_SEVERITY_KEY)
        .getOrElse(CPU_SEVERITY_DEFAULT);
  }

  static const constexpr char* NAME = "CpuContentionStrategy";

  static const constexpr char* DEFAULT_CPU_SEVERITY_KEY =
    "DEFAULT_CPU_SEVERITY";

 private:
  const lambda::function<usage::GetterFunction> getCpuUsage;

  // cfg parameters.
  double_t cfgDefaultSeverity;

  static constexpr double_t CPU_SEVERITY_DEFAULT = 1.0;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_STRATEGIES_CPU_CONTENTION_HPP
