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

Try<QoSCorrections> CacheOccupancyStrategy::decide(
    ExecutorAgeFilter* ageFilter,
    const Contentions& contentions,
    const ResourceUsage& usage) {
  std::vector<ResourceUsage_Executor> cmtEnabledExecutors =
      getCmtEnabledExecutors(usage);

  if (cmtEnabledExecutors.empty()) {
    return QoSCorrections();
  }

  double_t meanCacheOccupancy = countMeanCacheOccupancy(cmtEnabledExecutors);
  std::vector<ResourceUsage_Executor> aggressors =
    getExecutorsAboveMeanCacheOccupancy(cmtEnabledExecutors,
                                        meanCacheOccupancy);

  QoSCorrections corrections;
  for (auto aggressor : aggressors) {
    ExecutorInfo executorInfo = aggressor.executor_info();
    corrections.push_back(createKillQosCorrection(executorInfo));
  }

  return corrections;
}

std::vector<ResourceUsage_Executor>
CacheOccupancyStrategy::getCmtEnabledExecutors(
    const ResourceUsage& usage) const {
  std::vector<ResourceUsage_Executor> executors;
  for (const ResourceUsage_Executor& executor : usage.executors()) {
    if (executor.has_statistics() &&
        executor.statistics().has_perf() &&
        executor.statistics().perf().has_llc_occupancy()) {
      executors.push_back(executor);
    }
  }
  return executors;
}

double_t CacheOccupancyStrategy::countMeanCacheOccupancy(
    const std::vector<ResourceUsage_Executor> &_executors) const {
  double_t cacheOccupacySum = 0.0;
  for (const ResourceUsage_Executor& executor : _executors) {
    cacheOccupacySum += executor.statistics().perf().llc_occupancy();
  }
  return cacheOccupacySum / _executors.size();
}

std::vector<ResourceUsage_Executor>
CacheOccupancyStrategy::getExecutorsAboveMeanCacheOccupancy(
    const std::vector<ResourceUsage_Executor>& _executors,
    const double_t _cacheOccupancyMean) const {
  std::vector<ResourceUsage_Executor> product;
  for (const ResourceUsage_Executor& executor : _executors) {
    if (executor.statistics().perf().llc_occupancy() >= _cacheOccupancyMean) {
      product.push_back(executor);
    }
  }
  return product;
}

}  // namespace serenity
}  // namespace mesos
