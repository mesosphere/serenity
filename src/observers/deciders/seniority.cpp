#include <list>
#include <utility>

#include "observers/deciders/seniority.hpp"

namespace mesos {
namespace serenity {

using std::list;
using std::pair;

Try<QoSCorrections> SeniorityDecider::decide(
  ExecutorAgeFilter* ageFilter,
  const Contentions& currentContentions,
  const ResourceUsage& currentUsage) {
  // Product.
  QoSCorrections corrections;

  // List of BE executors.
  list<ResourceUsage_Executor> possibleAggressors =
    filterPrExecutors(currentUsage);

  // Agressors to be killed.
  std::list<slave::QoSCorrection_Kill> aggressorsToKill;

  double meanSeverity = 0.0;
  for (const Contention contention : currentContentions) {
    if (contention.has_aggressor()) {
      // Find specified aggressor and push it to the aggressors list.
      possibleAggressors.remove_if(
        [&contention, &aggressorsToKill]
          (ResourceUsage_Executor& possibleAggressor) {
          if (WID(contention.aggressor())
              != WID(possibleAggressor.executor_info())) {
            return false;
          }

          aggressorsToKill.push_back(
            createKill(possibleAggressor.executor_info()));

          return true;
        });
      continue;
    }

    if (contention.has_severity()) {
      meanSeverity += contention.severity();
    }
  }

  if (!currentContentions.empty()) {
    meanSeverity /= currentContentions.size();
  }
  LOG(INFO) << "MeanSeverity: " << meanSeverity;
  // TODO(nnielsen): Made gross assumption about homogenous best-effort tasks.
  // TODO(nnielsen): Instead of severity, we need taget values (corrections may
  // not have the desired effect). Keep correcting until we have 0 BE tasks.
  size_t killCount = possibleAggressors.size() * meanSeverity;
  if (killCount == 0 && (possibleAggressors.size() * meanSeverity) > 0) {
    killCount++;
  }

  LOG(INFO) << "Decided to kill " << killCount << "/"
  << possibleAggressors.size() << "executors";
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

  // Kill first N executors (by age, youngest to oldest).
  list<pair<double_t, ResourceUsage_Executor>>::iterator executorIterator =
                                                           executors.begin();
  for (int i = 0; i < killCount; i++) {
    if (executorIterator == executors.end()) {
      break;
    }

    const ExecutorInfo& executorInfo =
      (executorIterator->second).executor_info();

    LOG(INFO) << "Marked executor '" << executorInfo.executor_id()
    << "' of framework '" << executorInfo.framework_id()
    << "' age " << executorIterator->first << "s for removal";

    aggressorsToKill.push_back(createKill(executorInfo));
    executorIterator++;
  }

  // Create QoSCorrection from aggressors list.
  for (auto aggressorToKill : aggressorsToKill) {
    corrections.push_back(createKillQoSCorrection(aggressorToKill));
  }

  return corrections;
}

}  // namespace serenity
}  // namespace mesos
