#include <list>
#include <utility>
#include <vector>

#include "bus/event_bus.hpp"

#include "observers/strategies/seniority.hpp"

#include "serenity/resource_helper.hpp"

#include "cache_occupancy.hpp"

namespace mesos {
namespace serenity {

using std::list;
using std::pair;

/**
 * Rough draft of strategy:
 *
 * 1: Count mean cache occupation of BE executors
 * 2: Sort BE tasks on llc_occupancy
 * 3: Put BE into buckets:
 *    0) above mean llc_occupancy (inclusive)
 *    1) bellow mean llc_occupancy
 *    2) no lcc_alloc in statistics
 * 4: If bucket 0 has any - revoke "above" bucket and return
 * 5: else, sort bucket 3 by seniority, revoke X number of
 *    youngest containers and return
 */
Try<QoSCorrections> CacheOccupancyStrategy::decide(
    ExecutorAgeFilter* ageFilter,
    const Contentions& contentions,
    const ResourceUsage& usage) {

  double_t cacheOccuancySum = 0.0;
  uint32_t cmtEnabledExecutorsCount = 0;
  std::vector<ResourceUsage_Executor> cmtEnabledRevocableExecutors;
  for (const ResourceUsage_Executor& executor : usage.executors()) {
    if (executor.has_statistics() &&
        executor.statistics().has_perf() &&
        executor.statistics().perf().has_llc_occupancy()) {
      cacheOccuancySum += executor.statistics().perf().llc_occupancy();
      cmtEnabledExecutorsCount += 1;

      Try<bool> isExecutorRevocable = isBeExecutor(executor);
      if (isExecutorRevocable.isSome() && isExecutorRevocable.get()) {
        cmtEnabledRevocableExecutors.push_back(executor);
      }
    }
  }

  std::vector<ResourceUsage_Executor> aggressors;
  if (!cmtEnabledRevocableExecutors.empty()) {
    double_t cacheOccupancyMean = cacheOccuancySum / cmtEnabledExecutorsCount;
    aggressors = getCacheNoisyExecutors(cmtEnabledRevocableExecutors,
                                        cacheOccupancyMean);
  }

  QoSCorrections corrections;
  for (auto aggressor : aggressors) {
    ExecutorInfo executorInfo = aggressor.executor_info();
    corrections.push_back(createKillQosCorrection(executorInfo));
  }

  return corrections;
}


std::vector<ResourceUsage_Executor>
CacheOccupancyStrategy::getCacheNoisyExecutors(
    const std::vector<ResourceUsage_Executor>& _executors,
    double_t _cacheOccupancyMean) const {
  std::vector<ResourceUsage_Executor> product;
  for (const ResourceUsage_Executor& executor : _executors) {
    if (executor.statistics().perf().llc_occupancy() >= _cacheOccupancyMean) {
      product.push_back(executor);
    }
  }

  return product;
}


//std::vector<ResourceUsage_Executor>
//CacheOccupancyStrategy::getYoungestExecutors(
//    const std::vector<ResourceUsage_Executor>& executors,
//    ExecutorAgeFilter* ageFilter,
//    uint32_t count) const {
//  std::vector<ResourceUsage_Executor> product;
//  return product;
//}


}  // namespace serenity
}  // namespace mesos
