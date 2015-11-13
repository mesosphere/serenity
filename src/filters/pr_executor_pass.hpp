#ifndef SERENITY_PR_EXECUTOR_PASS_FILTER_HPP
#define SERENITY_PR_EXECUTOR_PASS_FILTER_HPP

#include <list>
#include <string>

#include "mesos/resources.hpp"

#include "messages/serenity.hpp"

#include "serenity/serenity.hpp"

#include "stout/lambda.hpp"
#include "stout/nothing.hpp"

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

/**
 * Filter retaining ResourceUsage for production executors only.
 * Additionally it can also produce ResourceUsage with Be tasks only
 * (as BeResourceUsage).
 */
class PrExecutorPassFilter :
    public Consumer<ResourceUsage>,
    public Producer<ResourceUsage> {
 public:
  explicit PrExecutorPassFilter(Consumer<ResourceUsage>* _consumer)
      : Producer<ResourceUsage>(_consumer) {}

  ~PrExecutorPassFilter() {}

  Try<Nothing> consume(const ResourceUsage& in);

  static constexpr const char* name = "[Serenity] PrExecutorPasFilter: ";
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_PR_TASKS_FILTER_HPP
