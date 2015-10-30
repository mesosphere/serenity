#ifndef SERENITY_CONTENTION_DECIDER_BASE_HPP
#define SERENITY_CONTENTION_DECIDER_BASE_HPP

#include <list>
#include <string>

#include "filters/executor_age.hpp"

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "messages/serenity.hpp"

#include "serenity/serenity.hpp"

#include "stout/try.hpp"

namespace mesos {
namespace serenity {

using ContentionDeciderFunction = Try<QoSCorrections>
    (ExecutorAgeFilter* ageFilter,
    const Contentions& currentContentions,
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

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_CONTENTION_DECIDER_BASE_HPP
