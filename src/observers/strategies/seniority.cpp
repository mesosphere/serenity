#include <list>
#include <string>
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

  // List of BE executors.
  list<ResourceUsage_Executor> possibleAggressors =
  ResourceUsageHelper::getRevocableExecutors(currentUsage);

  // Aggressors to be killed. (empty for now).
  std::list<slave::QoSCorrection_Kill> aggressorsToKill;

  double_t maxSeverity = this->severity;
  for (const Contention contention : currentContentions) {
    if (contention.has_severity()) {
      if (contention.severity() > maxSeverity) {
        maxSeverity = contention.severity();
      }
    }
  }

  // TODO(nnielsen): Made gross assumption about homogenous best-effort tasks.
  size_t executorsToRevokeCnt = ceil(possibleAggressors.size() * maxSeverity);
  if (executorsToRevokeCnt == 0) {
    return QoSCorrections();
  }

  // Get ages for executors.
  list<pair<double_t, ResourceUsage_Executor>> executors;
  for (const ResourceUsage_Executor& executor : possibleAggressors) {
    Try<double_t> age = ageFilter->age(executor.executor_info());
    if (age.isError()) {
      LOG(WARNING) << age.error();
      continue;
    }
    executors.push_back(
      pair<double_t, ResourceUsage_Executor>(age.get(), executor));
  }

  // TODO(nielsen): Actual time delta should be factored in i.e. not only work
  // as an ordering, but as a priority (taken time gaps).
  executors.sort([](
    const pair<double_t, ResourceUsage_Executor>& left,
    const pair<double_t, ResourceUsage_Executor>& right){
    return left.first < right.first;
  });

  QoSCorrections corrections;
  SERENITY_LOG(INFO) << "Revoking " << executorsToRevokeCnt << " executors";
  for (const auto& pair : executors) {
    slave::QoSCorrection correction =
      createKillQosCorrection(pair.second.executor_info());
    corrections.push_back(correction);

    std::string executorName = pair.second.executor_info().name();
    SERENITY_LOG(INFO) << "Marked " << executorName << "to revoke";

    executorsToRevokeCnt -= 1;
    if (executorsToRevokeCnt == 0) {
      break;
    }
  }

  return corrections;
}

}  // namespace serenity
}  // namespace mesos
