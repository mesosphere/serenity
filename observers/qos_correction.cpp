#include <list>
#include <vector>

#include "observers/qos_correction.hpp"

#include "mesos/slave/oversubscription.hpp"

#include "messages/serenity.hpp"

#include "serenity/wid.hpp"

namespace mesos {
namespace serenity {

using std::list;

/**
 * Checks contentions and choose executors to kill.
 * Currently we have sorted Contentions by severity.
 * We are iterating from the most serious contention to the less serious.
 * WARNING(bplotka): This interpreter assumes that severity is reflecting
 * how many resources is needed for PR job to not starve again.
 */
Try<QoSCorrections> ContentionInterpreters::severityBasedCpuContention(
    Contentions currentContentions,
    ResourceUsage currentUsage) {
  // Product.
  QoSCorrections corrections;

  // BE tasks.
  list<ResourceUsage_Executor> possibleAggressors =
      filterPrExecutors(currentUsage);

  // This interpreter controls CPU Contentions only.
  // TODO(bplotka): Consider sorting them by CpuUsage
  possibleAggressors.sort(QoSCorrectionObserver::compareCpuAllocated);

  // To not rescue the same PR executor more then one time.
  std::list<WID> correctedPrExecutors;
  // We save here aggressors to be killed.
  std::list<slave::QoSCorrection_Kill> aggressorsToKill;
  float_t severity_balance = 0;
  // Iterate over all contentions and assure that all PR contentions are
  // eliminated.
  for (const Contention contention : currentContentions) {
    float_t severity = 0.1;
    if (!contention.has_severity()) {
      LOG(INFO) << "Assuming that no severity field means lowest severity.";
      // In such case we kill most active Be task to ensure QoS and solve
      // this contention.
    } else {
      severity = contention.severity();
    }

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
      LOG(ERROR) << "Other contention types than CPU is not supported in "
                    "this interpeter.";
      continue;
    }

    if (contention.has_aggressor()) {
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

                LOG(INFO) << "Decided to kill Executor: " <<
                possibleAggressor.executor_info().executor_id();

                aggressorsToKill.push_back(
                    createKill(possibleAggressor.executor_info()));
              }

              return true;
            });
      }

      if (severity > 0) {
        // In such case even if we kill all BE executors contention is not
        // solved.
        LOG(ERROR) << "Aggressors are not the cause of CPU contention"
            " or lack of info about some aggressors.";
        break;
      }

      // We often kill executor with usage > severity. Keep this balance
      // for another contentions.
      severity_balance += severity;
    }
    // Eliminating duplicate contentions.
    correctedPrExecutors.push_back(WID(contention.victim()));
  }

  // Create QoSCorrection having aggressors list.
  for (auto aggressorToKill : aggressorsToKill) {
    corrections.push_back(createKillQoSCorrection(aggressorToKill));
  }

  return corrections;
}


QoSCorrectionObserver::~QoSCorrectionObserver() {}


Try<Nothing> QoSCorrectionObserver::_consume(
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
  CHECK_ERR(this->currentContentions.isSome());
  CHECK_ERR(this->currentUsage.isSome());

  if (this->currentContentions.get().empty()) {
    produce(QoSCorrections());
  } else {
    // Allowed to interpret contention using different algorithms.
    Try<QoSCorrections> corrections =
        this->interpretContention(this->currentContentions.get(),
                                  this->currentUsage.get());
    if (corrections.isError()) {
      // TODO(bplotka) Error handling behaviours need to be defined.
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
