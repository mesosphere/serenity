#include <list>
#include <vector>

#include "observers/qos_correction.hpp"

#include "mesos/slave/oversubscription.hpp"

#include "messages/serenity.hpp"

#include "serenity/wid.hpp"

namespace mesos {
namespace serenity {

using std::list;
using std::pair;

Try<QoSCorrections> SeverityBasedCpuDecider::decide(
    ExecutorAgeFilter* ageFilter,
    const Contentions& currentContentions,
    const ResourceUsage& currentUsage) {
  // Product.
  QoSCorrections corrections;

  // List of BE executors.
  list<ResourceUsage_Executor> possibleAggressors =
      filterPrExecutors(currentUsage);

  // This interpreter controls CPU Contentions only.
  // TODO(bplotka): Consider sorting them by CpuUsage
  possibleAggressors.sort(QoSCorrectionObserver::compareCpuAllocated);

  // We save PR executors here to not rescue them more then one time.
  std::list<WID> correctedPrExecutors;
  // We save here aggressors to be killed.
  std::list<slave::QoSCorrection_Kill> aggressorsToKill;
  // We save here surplus of severity for next contentions.
  float_t severity_balance = 0;

  // Iterate over all contentions and assure that all PR contentions are
  // eliminated.
  for (const Contention contention : currentContentions) {
    std::_List_iterator<WID> correctedExecutor =
      std::find(correctedPrExecutors.begin(),
                correctedPrExecutors.end(),
                WID(contention.victim()));
    if (correctedExecutor != correctedPrExecutors.end()) {
      // This victim is already spotted and correction has been
      // made.
      continue;
    }

    if (contention.type() != Contention_Type_CPU) {
      LOG(ERROR) << QoSCorrectionObserver::name <<
                    "Other contention types than CPU is "
                    "not supported in this interpeter.";
      continue;
    }

    // Case when aggressor is specified.
    if (contention.has_aggressor()) {
      // Find specified aggressor and push it to the aggressors list.
      possibleAggressors.remove_if(
          [&contention, &aggressorsToKill]
              (ResourceUsage_Executor& possibleAggressor) {
            if (WID(contention.aggressor())
                != WID(possibleAggressor.executor_info()))
              return false;

            aggressorsToKill.push_back(
                createKill(possibleAggressor.executor_info()));
            return true;
          });

      continue;
    } else {
      float_t severity = 0.1;
      if (!contention.has_severity()) {
        LOG(INFO) << QoSCorrectionObserver::name <<
                    "Got contention without severity being specified. "
                    "Assuming that no severity field means lowest severity.";
        // In such case we kill most active Be task to ensure QoS and solve
        // this contention.
      } else {
        severity = contention.severity();
      }

      severity += severity_balance;
      if (severity > 0) {
        // Assuming that severity is reflecting how many
        // resources is needed for PR job to not starve again.
        // TODO(bplotka) We could implement here more sophisticated algorithm
        // e.g: solving discrete knapsack problem.
        // Currently we are killing most active BE tasks first.
        // (Greedy behaviour).
        possibleAggressors.remove_if(
            [&severity, &aggressorsToKill]
                (ResourceUsage_Executor& possibleAggressor) {
              if (severity <= 0)
                return false;

              Resources aggressorsAllocation(possibleAggressor.allocated());
              if (aggressorsAllocation.cpus().isSome()) {
                severity -= aggressorsAllocation.cpus().get();

                LOG(INFO) << QoSCorrectionObserver::name
                          <<"Decided to kill ""Executor: "
                          <<possibleAggressor.executor_info().executor_id();

                aggressorsToKill.push_back(
                    createKill(possibleAggressor.executor_info()));
              }

              return true;
            });
      }

      if (severity > 0) {
        // In such case even if we kill all BE executors contention is not
        // solved. That could mean we badly estimated severity or
        // aggressors are not the cause of CPU contention.
        LOG(INFO) << QoSCorrectionObserver::name <<
                      "Aggressors are not the cause of CPU contention"
                      " or lack of info about some aggressors.";
        break;
      }

      // We often kill executor with usage > severity. Keep this surplus
      // for another contentions.
      severity_balance += severity;
    }
    // Eliminating duplicate contentions.
    correctedPrExecutors.push_back(WID(contention.victim()));
  }

  // Create QoSCorrection from aggressors list.
  for (auto aggressorToKill : aggressorsToKill) {
    corrections.push_back(createKillQoSCorrection(aggressorToKill));
  }

  return corrections;
}


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


// Using age filter.
Try<QoSCorrections> SeverityBasedSeniorityDecider::decide(
    ExecutorAgeFilter* ageFilter,
    const Contentions& currentContentions,
    const ResourceUsage& currentUsage) {
  // Product.
  QoSCorrections corrections;

  // List of BE executors.
  list<ResourceUsage_Executor> possibleAggressors =
      filterPrExecutors(currentUsage);

  // We save here aggressors to be killed.
  std::list<slave::QoSCorrection_Kill> aggressorsToKill;

  double meanSeverity = 0.0;
  for (const Contention contention : currentContentions) {
    if (contention.has_severity()) {
      meanSeverity += contention.severity();
    }
  }
  
  if (!currentContentions.empty()) {
    meanSeverity /= currentContentions.size();
  }
  LOG(INFO) << meanSeverity;
  // TODO(nnielsen): Made gross assumption about homogenous best-effort tasks.
  // TODO(nnielsen): Instead of severity, we need taget values (corrections may
  // not have the desired effect). Keep correcting until we have 0 BE tasks.
  size_t killCount = possibleAggressors.size() * meanSeverity;

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

    const ExecutorInfo& executorInfo = (executorIterator->second).executor_info();

    LOG(INFO) << "Marked executor '" << executorInfo.executor_id()
              << "' of framework '" << executorInfo.framework_id()
              << "' age " << executorIterator->first << "s for removal";

    corrections.push_back(createKillQoSCorrection(createKill(executorInfo)));

    executorIterator++;
  }

  return corrections;

#if 0
  // Iterate over all contentions and assure that all PR contentions are
  // eliminated.
  for (const Contention contention : currentContentions) {
    std::_List_iterator<WID> correctedExecutor =
      std::find(correctedPrExecutors.begin(),
                correctedPrExecutors.end(),
                WID(contention.victim()));
    if (correctedExecutor != correctedPrExecutors.end()) {
      // This victim is already spotted and correction has been
      // made.
      continue;
    }

    if (contention.type() != Contention_Type_CPU) {
      LOG(ERROR) << QoSCorrectionObserver::name <<
                    "Other contention types than CPU is "
                    "not supported in this interpeter.";
      continue;
    }

    // Case when aggressor is specified.
    if (contention.has_aggressor()) {
      // Find specified aggressor and push it to the aggressors list.
      possibleAggressors.remove_if(
          [&contention, &aggressorsToKill]
              (ResourceUsage_Executor& possibleAggressor) {
            if (WID(contention.aggressor())
                != WID(possibleAggressor.executor_info()))
              return false;

            aggressorsToKill.push_back(
                createKill(possibleAggressor.executor_info()));
            return true;
          });

      continue;
    } else {
      float_t severity = 0.1;
      if (!contention.has_severity()) {
        LOG(INFO) << QoSCorrectionObserver::name <<
                    "Got contention without severity being specified. "
                    "Assuming that no severity field means lowest severity.";
        // In such case we kill most active Be task to ensure QoS and solve
        // this contention.
      } else {
        severity = contention.severity();
      }

      severity += severity_balance;
      if (severity > 0) {
        // Assuming that severity is reflecting how many
        // resources is needed for PR job to not starve again.
        // TODO(bplotka) We could implement here more sophisticated algorithm
        // e.g: solving discrete knapsack problem.
        // Currently we are killing most active BE tasks first.
        // (Greedy behaviour).
        possibleAggressors.remove_if(
            [&severity, &aggressorsToKill]
                (ResourceUsage_Executor& possibleAggressor) {
              if (severity <= 0)
                return false;

              Resources aggressorsAllocation(possibleAggressor.allocated());
              if (aggressorsAllocation.cpus().isSome()) {
                severity -= aggressorsAllocation.cpus().get();

                LOG(INFO) << QoSCorrectionObserver::name
                          << "Decided to kill ""Executor: "
                          << possibleAggressor.executor_info().executor_id();

                aggressorsToKill.push_back(
                    createKill(possibleAggressor.executor_info()));
              }

              return true;
            });
      }

      if (severity > 0) {
        // In such case even if we kill all BE executors contention is not
        // solved. That could mean we badly estimated severity or
        // aggressors are not the cause of CPU contention.
        LOG(INFO) << QoSCorrectionObserver::name <<
                      "Aggressors are not the cause of CPU contention"
                      " or lack of info about some aggressors.";
        break;
      }

      // We often kill executor with usage > severity. Keep this surplus
      // for another contentions.
      severity_balance += severity;
    }
    // Eliminating duplicate contentions.
    correctedPrExecutors.push_back(WID(contention.victim()));
  }
#endif
}


QoSCorrectionObserver::~QoSCorrectionObserver() {}


Try<Nothing> QoSCorrectionObserver::_syncConsume(
  const std::vector<Contentions> products) {
  Contentions newContentions;

  for (Contentions contentions : products)
    for (Contention contention : contentions)
      newContentions.push_back(contention);

  this->currentContentions = newContentions;
  this->currentContentions.get().sort(compareSeverity);

  if (this->currentUsage.isSome()) {
    this->__correctSlave();
  }

  return Nothing();
}


Try<Nothing> QoSCorrectionObserver::consume(const ResourceUsage& usage) {
  this->currentUsage = usage;

  if (this->currentContentions.isSome()) {
    this->__correctSlave();
  }

  return Nothing();
}


Try<Nothing> QoSCorrectionObserver::__correctSlave() {
  // Consumer base code ensures we have needed information here.
  if (!this->currentContentions.isSome() || !this->currentUsage.isSome())
    return Nothing();

  if (this->currentContentions.get().empty()) {
    produce(QoSCorrections());
  } else {
    // Allowed to interpret contention using different algorithms.
    Try<QoSCorrections> corrections =
        this->contentionDecider->decide(ageFilter,
					this->currentContentions.get(),
                                        this->currentUsage.get());
    if (corrections.isError()) {
      // In case of Error produce empty corrections.
      produce(QoSCorrections());
    } else {
      produce(corrections.get());
    }
  }

  this->currentContentions = None();
  this->currentUsage = None();

  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
