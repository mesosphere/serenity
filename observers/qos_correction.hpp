#ifndef SERENITY_QOS_CORRECTION_HPP
#define SERENITY_QOS_CORRECTION_HPP

#include <list>
#include <vector>

#include "glog/logging.h"

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "mesos/slave/oversubscription.hpp"

#include "messages/serenity.hpp"

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

inline std::list<ResourceUsage_Executor> filterPrExecutors(
    ResourceUsage usage) {
  std::list<ResourceUsage_Executor> beExecutors;
  for (ResourceUsage_Executor inExec : usage.executors()) {
    if (!inExec.has_executor_info()) {
      LOG(ERROR) << "Executor <unknown>"
      << " does not include executor_info";
      // Filter out these executors.
      continue;
    }
    if (inExec.allocated().size() == 0) {
      LOG(ERROR) << "Executor "
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


using ContentionInterpreterFunction = Try<QoSCorrections>
    (Contentions currentContentions,
    ResourceUsage currentUsage);


/**
 * Class including all algorithms for contention interpretations.
 * They converts contentions & usage to QoSCorrections.
 *
 * Convenient for debuging and testing different algorithms.
 */
class ContentionInterpreters {
 public:
  static ContentionInterpreterFunction severityBasedCpuContention;
};


/**
 * QoSCorrectionObserver observes incoming Contentions and
 * ResourceUsage and produces QoSCorrections.
 *
 * Currently Slave understand only kill correction action.
 */
class QoSCorrectionObserver : public MultiConsumer<Contentions>,
                              public Consumer<ResourceUsage>,
                              public Producer<QoSCorrections> {
 public:
  explicit QoSCorrectionObserver(
      Consumer<QoSCorrections>* _consumer,
      uint64_t _contentionProducents,
      const lambda::function<
          ContentionInterpreterFunction>& _interpretContention =
        ContentionInterpreters::severityBasedCpuContention)
    : MultiConsumer<Contentions>(_contentionProducents),
      Producer<QoSCorrections>(_consumer),
      currentContentions(None()),
      currentUsage(None()),
      interpretContention(_interpretContention) {}

  ~QoSCorrectionObserver();

  Try<Nothing> _consume(const std::vector<Contentions> products) override;

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

 protected:
  Option<Contentions> currentContentions;
  Option<ResourceUsage> currentUsage;
  const lambda::function<ContentionInterpreterFunction>& interpretContention;

  //! Run when all required info are gathered.
  Try<Nothing> __correctSlave();
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_QOS_CORRECTION_HPP
