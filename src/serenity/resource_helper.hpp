#ifndef SERENITY_RESOURCE_HELPER_HPP
#define SERENITY_RESOURCE_HELPER_HPP

#include <list>

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

namespace mesos {
namespace serenity {

/**
 * Useful class for having dividing usage to Production & Best Effort
 * executors.
 */
class DividedResourceUsage {
 public:
  explicit DividedResourceUsage(const ResourceUsage& _usage) {
    usage.CopyFrom(_usage);

    for (ResourceUsage_Executor executor : usage.executors()) {
      // Check if task uses revocable resources.
      Resources allocated(executor.allocated());
      if (allocated.revocable().empty()) {
        pr.push_back(executor);
      } else {
        be.push_back(executor);
      }
    }
  }

  /**
   * Static helper for filtering Executors.
   */
  static inline std::list<ResourceUsage_Executor> filterPrExecutors(
    const ResourceUsage& usage) {
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

  const std::list<ResourceUsage_Executor>& prExecutors() const {
    return this->pr;
  }

  const std::list<ResourceUsage_Executor>& beExecutors() const {
    return this->be;
  }

  const Resources total() const {
    return this->usage.total();
  }

 protected:
  std::list<ResourceUsage_Executor> pr, be;
  ResourceUsage usage;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_RESOURCE_HELPER_HPP
