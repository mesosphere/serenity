#ifndef SERENITY_RESOURCE_HELPER_HPP
#define SERENITY_RESOURCE_HELPER_HPP

#include <list>

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

namespace mesos {
namespace serenity {

class DividedResourceUsage {
public:
  DividedResourceUsage(const ResourceUsage& _usage) {
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

  const std::list<ResourceUsage_Executor>& prExecutors() const {
    return this->pr;
  }

  const std::list<ResourceUsage_Executor>& beExecutors() const {
    return this->be;
  }

  const Resources& total() const {
    this->usage.total();
  }

protected:
  std::list<ResourceUsage_Executor> pr, be;
  ResourceUsage usage;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_RESOURCE_HELPER_HPP
