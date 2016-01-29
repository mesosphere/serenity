#ifndef SERENITY_RESOURCE_HELPER_HPP
#define SERENITY_RESOURCE_HELPER_HPP

#include <list>
#include <tuple>

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

namespace mesos {
namespace serenity {

class ResourceUsageHelper {
 public:
  /**
   * Note: Drops executors that does not have allocated resources.
   */
  static std::list<ResourceUsage_Executor> getRevocableExecutors(
    const ResourceUsage&);

  /**
   * Note: Drops executors that does not have allocated resources.
   */
  static std::list<ResourceUsage_Executor> getProductionExecutors(
    const ResourceUsage&);

  /**
   * Returns tuple of <list<Production>, list<Revocable>> executors.
   * Note, that it drops executors that does not have allocated
   * resources.
   */
  static std::tuple<std::list<ResourceUsage_Executor>,
                    std::list<ResourceUsage_Executor>>
      getProductionAndRevocableExecutors(const ResourceUsage&);

  /**
  * Checks if executor has empty revocable resources.
  *
  * Returns error when executor has no allocated resources.
  */
  static Try<bool> isProductionExecutor(const ResourceUsage_Executor&);

  /**
  * Checks if executor has revocable resources.
  *
  * Returns error when executor has no allocated resources.
  */
  static Try<bool> isRevocableExecutor(const ResourceUsage_Executor&);

  static bool isExecutorHasStatistics(const ResourceUsage_Executor&);

  enum ExecutorType : int {
    PRODUCTION = 0,
    REVOCABLE = 1
  };
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_RESOURCE_HELPER_HPP
