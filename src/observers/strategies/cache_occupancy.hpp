#ifndef SERENITY_CACHE_OCCUPANCY_HPP
#define SERENITY_CACHE_OCCUPANCY_HPP

#include <list>
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
  CacheOccupancyStrategy() : RevocationStrategy(Tag(QOS_CONTROLLER, NAME)) {
    init();
  }

  explicit CacheOccupancyStrategy(const Config& _config)
  : RevocationStrategy(Tag(QOS_CONTROLLER, NAME)) {
    init();
  }

  Try<QoSCorrections> decide(ExecutorAgeFilter* ageFilter,
                             const Contentions& currentContentions,
                             const ResourceUsage& currentUsage);

  static const constexpr char* NAME = "CacheOccupancyStrategy";


 protected:
  void init() {
    minimalCacheOccupancy = MINIMAL_CACHE_OCCUPANCY_DEFAULT;
  }

  std::vector<ResourceUsage_Executor> getCmtEnabledExecutors(
    const std::list<ResourceUsage_Executor>&) const;

  double_t countMeanCacheOccupancy(
    const std::vector<ResourceUsage_Executor>&) const;

  std::vector<ResourceUsage_Executor> getExecutorsAboveMinimalAndMeanOccupancy(
    const std::vector<ResourceUsage_Executor>& executors,
    const double_t meanCacheOccupancy) const;

  //!< Minimal cache occupancy for executor to be revoked.
  static constexpr uint64_t MINIMAL_CACHE_OCCUPANCY_DEFAULT = 1000000;  // 1M
  uint64_t minimalCacheOccupancy;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_CACHE_OCCUPANCY_HPP
