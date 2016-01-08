#include <algorithm>
#include <list>
#include <utility>

#include "bus/event_bus.hpp"

#include "filters/executor_age.hpp"

#include "observers/strategies/cpu_contention.hpp"

#include "serenity/resource_helper.hpp"

namespace mesos {
namespace serenity {

using std::list;
using std::pair;

Try<QoSCorrections> CpuContentionStrategy::decide(
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
      StaticEventBus::publishOversubscriptionCtrlEvent(true);
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
      SERENITY_LOG(INFO) << "Cooldown active. Iterations to go: "
                         << cooldownCounterValue;

      return QoSCorrections();
    } else {
      SERENITY_LOG(INFO) << "Cooldown ended, but we have still contention"
        "situation! Starting new revocations.";
    }
  }

  // ---!!!--- Contention spotted - start new cooldown. ---!!!---
  // TODO(bplotka): Change cooldownTime dynamically (learninig).
  cooldownCounter = this->cfgCooldownTime;

  // Disable Estimator pipeline.
  if (!estimatorDisabled) {
    StaticEventBus::publishOversubscriptionCtrlEvent(false);
    estimatorDisabled = true;
  }

  // Product.
  QoSCorrections corrections;

  // List of BE executors.
  list<ResourceUsage_Executor> possibleAggressors =
    DividedResourceUsage::filterPrExecutors(currentUsage);

  // Aggressors to be killed. (empty for now).
  std::list<slave::QoSCorrection_Kill> aggressorsToKill;

  // Amount of CPUs to satisfy the contention.
  double_t cpuToRecover = 0.0;
  for (const Contention contention : currentContentions) {
    if (contention.type() != Contention_Type_CPU) {
      SERENITY_LOG(ERROR) << "Cannot decide about contentions type other than"
                          << " CPU. Omitting instance";
    }

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

    // In case of Contention_Type_CPU severity value means amount of CPUs to
    // recover.
    if (contention.has_severity()) {
      cpuToRecover = std::max(contention.severity(), cpuToRecover);
    } else {
      cpuToRecover = std::max(this->cfgDefaultSeverity, cpuToRecover);
    }
  }

  SERENITY_LOG(INFO) << "Cpus to recover from revocable tasks: "
                     << cpuToRecover;

  // Check for cpus limits violations and set age.
  list<pair<double_t, ResourceUsage_Executor>> executors;
  for (const ResourceUsage_Executor& executor : possibleAggressors) {
    if (cpuToRecover <= 0) break;

    Try<double_t> value = this->cpuUsageGetFunction(executor);
    if (value.isError()) {
      SERENITY_LOG(ERROR) << value.error();
      continue;
    }

    if (value.get() > Resources(executor.allocated()).cpus().get()) {
      // Executor exceeds its limits, mark it for revocation.
      const ExecutorInfo& executorInfo = executor.executor_info();
      SERENITY_LOG(INFO) << "Marked executor '" << executorInfo.executor_id()
      << "' of framework '" << executorInfo.framework_id()
      << "' because of limit violation";

      aggressorsToKill.push_back(createKill(executorInfo));

      // Recover cpus.
      cpuToRecover -= value.get();
      continue;
    }

    Try<double_t> age = ageFilter->age(executor.executor_info());
    if (age.isError()) {
      LOG(WARNING) << age.error();
      continue;
    }

    executors.push_back(
      pair<double_t, ResourceUsage_Executor>(age.get(), executor));
  }

  if (cpuToRecover > 0) {
    // TODO(nielsen): Actual time delta should be factored in i.e. not only work
    // as an ordering, but as a priority (taken time gaps).
    executors.sort([](
      const pair<double_t, ResourceUsage_Executor>& left,
      const pair<double_t, ResourceUsage_Executor>& right){
      return left.first < right.first;
    });

    for (auto executor : executors) {
      if (cpuToRecover <= 0) break;

      const ExecutorInfo& executorInfo =
        (executor.second).executor_info();

      SERENITY_LOG(INFO) << "Marked executor '" << executorInfo.executor_id()
      << "' of framework '" << executorInfo.framework_id()
      << "' age " << executor.first << "s for removal";

      aggressorsToKill.push_back(createKill(executorInfo));

      // TODO(bplotka): Use Cpu Usage, EMA Cpu Usage or allocated cpus?
      // IMO EMA CPU usage is the most accurate here...
      Try<double_t> value = this->cpuUsageGetFunction(executor.second);
      double_t recoveredCpus = 0.0;
      if (value.isError()) {
        SERENITY_LOG(ERROR) << value.error();
        recoveredCpus = Resources(executor.second.allocated()).cpus().get();
      } else {
        recoveredCpus = value.get();
      }

      cpuToRecover -= recoveredCpus;
    }
  }

  // Create QoSCorrection from aggressors list.
  for (auto aggressorToKill : aggressorsToKill) {
    corrections.push_back(createKillQoSCorrection(aggressorToKill));
  }

  return corrections;
}

}  // namespace serenity
}  // namespace mesos
