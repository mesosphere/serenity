#ifndef SERENITY_CACHE_OCCUPANCY_H
#define SERENITY_CACHE_OCCUPANCY_H

#include <vector>

#include "glog/logging.h"

#include "observers/strategies/base.hpp"

#include "serenity/config.hpp"
#include "serenity/wid.hpp"


namespace mesos {
namespace serenity {

/**
 * Cach Occupancy Strategy looks at executor's LLC_OCCUPANCY and revokes
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
  std::vector<ResourceUsage_Executor> getCacheNoisyExecutors(
    const std::vector<ResourceUsage_Executor>& executors,
    double_t cacheOccupancySum) const;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_CACHE_OCCUPANCY_HPP
