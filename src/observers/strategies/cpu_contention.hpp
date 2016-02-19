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
      const SerenityConfig& _config,
      const lambda::function<usage::GetterFunction>& _cpuUsageGetFunction)
      : RevocationStrategy(Tag(QOS_CONTROLLER, "CpuContentionStrategy")),
        getCpuUsage(_cpuUsageGetFunction) {
    setDefaultSeverity(_config.getItemOrDefault<double_t>(
        strategy::DEFAULT_CPU_SEVERITY,
        strategy::DEFAULT_DEFAULT_CPU_SEVERITY));
  }

  Try<QoSCorrections> decide(ExecutorAgeFilter* ageFilter,
                             const Contentions& currentContentions,
                             const ResourceUsage& currentUsage);

  static const constexpr char* NAME = "CpuContentionStrategy";

  void setDefaultSeverity(double_t defaultSeverity) {
    CpuContentionStrategy::defaultSeverity = defaultSeverity;
  }

 private:
  const lambda::function<usage::GetterFunction> getCpuUsage;

  // cfg parameters.
  double_t defaultSeverity;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_STRATEGIES_CPU_CONTENTION_HPP
