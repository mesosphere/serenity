#include <list>
#include <utility>

#include "bus/event_bus.hpp"

#include "observers/strategies/seniority.hpp"

#include "serenity/resource_helper.hpp"

namespace mesos {
namespace serenity {

using std::list;
using std::pair;

Try<QoSCorrections> SeniorityStrategy::decide(
    ExecutorAgeFilter* ageFilter,
    const Contentions& currentContentions,
    const ResourceUsage& currentUsage) {
  // Product.
  QoSCorrections corrections;

  // List of BE executors.
  list<ResourceUsage_Executor> possibleAggressors =
  ResourceUsageHelper::getRevocableExecutors(currentUsage);

  // Aggressors to be killed. (empty for now).
  std::list<slave::QoSCorrection_Kill> aggressorsToKill;

  double_t maxSeverity = this->defaultSeverity;
  for (const Contention contention : currentContentions) {
    if (contention.has_severity()) {
      if (contention.severity() > maxSeverity) {
        maxSeverity = contention.severity();
      }
    }
  }
  // TODO(nnielsen): Made gross assumption about homogenous best-effort tasks.
  size_t killCount = ceil(possibleAggressors.size() * maxSeverity);

  // Get ages for executors.
  list<pair<double_t, ResourceUsage_Executor>> executors;
  for (const ResourceUsage_Executor& executor : possibleAggressors) {
    Try<double_t> age = ageFilter->age(executor.executor_info());
    if (age.isError()) {
      LOG(WARNING) << age.error();
      continue;
    }

    executors.push_back(pair<double_t, ResourceUsage_Executor>(age.get(),
                                                               executor));
  }

  // TODO(nielsen): Actual time delta should be factored in i.e. not only work
  // as an ordering, but as a priority (taken time gaps).
  executors.sort([](
    const pair<double_t, ResourceUsage_Executor>& left,
    const pair<double_t, ResourceUsage_Executor>& right){
    return left.first < right.first;
  });

  /// ----------- sort ages for executor //

  for (const auto& pair : executors) {
    if (killCount == 0) {
      break;
    }
    slave::QoSCorrection correction =
      createKillQosCorrection(pair.second.executor_info());
    corrections.push_back(correction);
    killCount -= 1;
  }

  return corrections;
}

}  // namespace serenity
}  // namespace mesos
