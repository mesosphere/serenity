#include <list>
#include <utility>

#include "bus/event_bus.hpp"

#include "observers/strategies/seniority.hpp"

#include "serenity/resource_helper.hpp"

namespace mesos {
namespace serenity {

using std::list;
using std::pair;

inline void setOpenEstimatorPipeline(bool opened) {
  OversubscriptionCtrlEventEnvelope envelope;
  envelope.mutable_message()->set_enable(opened);
  StaticEventBus::publish<OversubscriptionCtrlEventEnvelope>(envelope);
}

Try<QoSCorrections> SeniorityStrategy::decide(
    ExecutorAgeFilter* ageFilter,
    const Contentions& currentContentions,
    const ResourceUsage& currentUsage) {
  if (currentContentions.empty()) {
    // No contentions happened.
    // It means that no interference happens or just there are no BE tasks.

    // Reset cooldownCounter.
    if (cooldownCounter.isSome()) {
      cooldownCounter = None();
    }

    // Open Estimator Pipeline.
    if (estimatorDisabled) {
      setOpenEstimatorPipeline(true);
      estimatorDisabled = false;
    }

    // Return empty corrections;
    return QoSCorrections();
  }

  // Check if cooldown is enabled.
  if (cooldownCounter.isSome()) {
    // TODO(bplotka): Make a separate util cooldown timer.
    uint64_t cooldownCounterValue = cooldownCounter.get();
    cooldownCounterValue--;

    if (cooldownCounterValue > 0) {
      // Cooldown phase - nothing to correct.
      cooldownCounter = cooldownCounterValue;

      // Return empty corrections;
      return QoSCorrections();
    } else {
      SERENITY_LOG(INFO) << "Cooldown ended, but we have still contention"
        "situation! Starting new revocations.";
      // TODO(bplotka): Change defaultSeverity to steer the revocation number.
      // this->defaultSeverity++; ?
    }
  }

  // ---!!!--- Contention spotted - start new cooldown. ---!!!---
  // TODO(bplotka): Change cooldownTime dynamically (learninig).
  cooldownCounter = this->cfgCooldownTime;
  // Disable Estimator pipeline.
  if (!estimatorDisabled) {
    setOpenEstimatorPipeline(false);
    estimatorDisabled = true;
  }

  // Product.
  QoSCorrections corrections;

  // List of BE executors.
  list<ResourceUsage_Executor> possibleAggressors =
    DividedResourceUsage::filterPrExecutors(currentUsage);

  // Aggressors to be killed. (empty for now).
  std::list<slave::QoSCorrection_Kill> aggressorsToKill;

  double_t meanSeverity = 0;
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

    // Most of contention will not have severity specified since it's
    // difficult to match severity with number aggressors to kill!
    if (contention.has_severity()) {
      meanSeverity += contention.severity();
    } else {
      meanSeverity += this->cfgDefaultSeverity;
    }
  }

  meanSeverity /= currentContentions.size();

  SERENITY_LOG(INFO) << "MeanSeverity: " << meanSeverity;
  // TODO(nnielsen): Made gross assumption about homogenous best-effort tasks.
  size_t killCount = possibleAggressors.size() * meanSeverity;
  if (killCount == 0 && (possibleAggressors.size() * meanSeverity) > 0) {
    killCount++;
  }

  SERENITY_LOG(INFO) << "Decided to kill " << killCount << "/"
                     << possibleAggressors.size() << "executors";
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

  // Kill first N executors (by age, youngest to oldest).
  list<pair<double_t, ResourceUsage_Executor>>::iterator executorIterator =
                                                           executors.begin();
  for (int i = 0; i < killCount; i++) {
    if (executorIterator == executors.end()) {
      break;
    }

    const ExecutorInfo& executorInfo =
      (executorIterator->second).executor_info();

    SERENITY_LOG(INFO) << "Marked executor '" << executorInfo.executor_id()
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
