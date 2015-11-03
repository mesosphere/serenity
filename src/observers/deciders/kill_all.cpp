#include <list>

#include "observers/deciders/kill_all.hpp"

namespace mesos {
namespace serenity {

using std::list;

Try<QoSCorrections> KillAllDecider::decide(
  ExecutorAgeFilter* ageFilter,
  const Contentions& currentContentions,
  const ResourceUsage& currentUsage) {
  // Product.
  QoSCorrections corrections;

  // List of BE executors.
  list<ResourceUsage_Executor> aggressors =
    filterPrExecutors(currentUsage);

  // Create QoSCorrection from aggressors list.
  for (auto aggressorToKill : aggressors) {
    corrections.push_back(createKillQoSCorrection(
      createKill(aggressorToKill.executor_info())));
  }

  return corrections;
}

}  // namespace serenity
}  // namespace mesos
