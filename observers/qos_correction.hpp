#ifndef SERENITY_QOS_CORRECTION_HPP
#define SERENITY_QOS_CORRECTION_HPP

#include <list>
#include <string>
#include <vector>

#include "glog/logging.h"

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "mesos/slave/oversubscription.hpp"

#include "messages/serenity.hpp"

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

using ContentionDeciderFunction = Try<QoSCorrections>
    (const Contentions& currentContentions,
     const ResourceUsage& currentUsage);


/**
 * Convenient base class for contention interpretations.
 * It converts contentions & usage to QoSCorrections.
 *
 * Convenient for debugging and testing different algorithms.
 */
class ContentionDecider {
 public:
  virtual ContentionDeciderFunction decide = 0;
};


/**
 * Checks contentions and choose executors to kill.
 * Currently it takes sorted contentions by severity.
 * We are iterating from the most serious contention to the less serious
 * and choose BE executors to kill.
 * WARNING(bplotka): This interpreter algorithm assumes that severity is
 * reflecting how many resources is needed for PR job to not starve again.
 */
class SeverityBasedCpuDecider : public ContentionDecider {
 public:
  ContentionDeciderFunction decide;
};


/**
 * QoSCorrectionObserver observes incoming Contentions and
 * ResourceUsage and produces QoSCorrections.
 *
 * Currently Slave understand only kill correction action.
 */
class QoSCorrectionObserver : public SyncConsumer<Contentions>,
                              public Consumer<ResourceUsage>,
                              public Producer<QoSCorrections> {
 public:
  explicit QoSCorrectionObserver(
      Consumer<QoSCorrections>* _consumer,
      uint64_t _contentionProducents,
      ContentionDecider* _contentionDecider = new SeverityBasedCpuDecider())
    : SyncConsumer<Contentions>(_contentionProducents),
      Producer<QoSCorrections>(_consumer),
      currentContentions(None()),
      currentUsage(None()),
      contentionDecider(_contentionDecider) {}

  ~QoSCorrectionObserver();

  Try<Nothing> _syncConsume(const std::vector<Contentions> products) override;

  Try<Nothing> consume(const ResourceUsage& usage) override;

  static bool compareSeverity(
      const Contention& first, const Contention& second) {
    if (!second.has_severity())
      return true;
    if (!first.has_severity())
      return false;

    return (first.severity() > second.severity());
  }

  static bool compareCpuAllocated(
      const ResourceUsage_Executor& first,
      const ResourceUsage_Executor& second) {
    // TODO(bplotka) decide if we want to validate fields here.
    Resources secondAllocation(second.allocated());
    if (secondAllocation.cpus().isNone())
      return true;

    Resources firstAllocation(first.allocated());
    if (firstAllocation.cpus().isNone())
      return false;

    return (firstAllocation.cpus().get() > secondAllocation.cpus().get());
  }

  static constexpr const char* name = "[SerenityQoS] CorrectionObserver: ";

 protected:
  Option<Contentions> currentContentions;
  Option<ResourceUsage> currentUsage;
  ContentionDecider* contentionDecider;

  //! Run when all required info are gathered.
  Try<Nothing> __correctSlave();
};


inline std::list<ResourceUsage_Executor> filterPrExecutors(
    ResourceUsage usage) {
  std::list<ResourceUsage_Executor> beExecutors;
  for (ResourceUsage_Executor inExec : usage.executors()) {
    if (!inExec.has_executor_info()) {
      LOG(ERROR) << QoSCorrectionObserver::name << "Executor <unknown>"
      << " does not include executor_info";
      // Filter out these executors.
      continue;
    }
    if (inExec.allocated().size() == 0) {
      LOG(ERROR) << QoSCorrectionObserver::name << "Executor "
      << inExec.executor_info().executor_id().value()
      << " does not include allocated resources.";
      // Filter out these executors.
      continue;
    }

    Resources allocated(inExec.allocated());
    // Check if task uses revocable resources.
    if (!allocated.revocable().empty())
      beExecutors.push_back(inExec);
  }

  return beExecutors;
}


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_QOS_CORRECTION_HPP
