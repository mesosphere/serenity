#ifndef SERENITY_CACHE_OCCUPANCY_HPP
#define SERENITY_CACHE_OCCUPANCY_HPP

#include <vector>

#include "glog/logging.h"

#include "observers/strategies/base.hpp"

#include "serenity/config.hpp"
#include "serenity/wid.hpp"


namespace mesos {
namespace serenity {

/**
 * Cache Occupancy Strategy looks at executor's LLC_OCCUPANCY and revokes
 * revocable jobs that are above (inclusive) mean LLC_OCCUPANCY for node.
 *
 * It returns empty QoSCorrections when there is zero BE tasks that
 * has llc_occupancy field in perf statistics.
 */
class CacheOccupancyStrategy : public RevocationStrategy {
 public:
  CacheOccupancyStrategy() : RevocationStrategy(Tag(QOS_CONTROLLER, NAME)) {}

  explicit CacheOccupancyStrategy(const SerenityConfig& _config)
  : RevocationStrategy(Tag(QOS_CONTROLLER, NAME)) {}

  Try<QoSCorrections> decide(ExecutorAgeFilter* ageFilter,
                             const Contentions& currentContentions,
                             const ResourceUsage& currentUsage);

  static const constexpr char* NAME = "CacheOccupancyStrategy";
 protected:
  std::vector<ResourceUsage_Executor> getCmtEnabledExecutors(
    const ResourceUsage&) const;

  double_t countMeanCacheOccupancy(
    const std::vector<ResourceUsage_Executor>&) const;

  std::vector<ResourceUsage_Executor> getExecutorsAboveMeanCacheOccupancy(
    const std::vector<ResourceUsage_Executor>&,
    const double_t) const;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_CACHE_OCCUPANCY_HPP
