#include <list>
#include <string>
#include <utility>
#include <vector>

#include "bus/event_bus.hpp"

#include "observers/strategies/cache_occupancy.hpp"
#include "observers/strategies/seniority.hpp"

#include "serenity/resource_helper.hpp"


namespace mesos {
namespace serenity {

using std::list;
using std::pair;

Try<QoSCorrections> CacheOccupancyStrategy::decide(
    ExecutorAgeFilter* ageFilter,
    const Contentions& contentions,
    const ResourceUsage& usage) {

  std::vector<ResourceUsage_Executor> beCmtEnabledExecutors
    = getCmtEnabledExecutors(ResourceUsageHelper::getRevocableExecutors(usage));

  if (beCmtEnabledExecutors.empty()) {
    return QoSCorrections();
  }

  double_t meanCacheOccupancy = countMeanCacheOccupancy(beCmtEnabledExecutors);
  std::vector<ResourceUsage_Executor> aggressors =
  getExecutorsAboveMinimalAndMeanOccupancy(beCmtEnabledExecutors,
                                           meanCacheOccupancy);

  SERENITY_LOG(INFO) << "Revoking " << aggressors.size() << " executors";
  QoSCorrections corrections;
  for (auto aggressor : aggressors) {
    ExecutorInfo executorInfo = aggressor.executor_info();
    corrections.push_back(createKillQosCorrection(executorInfo));

    std::string executorName = aggressor.executor_info().name();
    SERENITY_LOG(INFO) << "Marked " << executorName << "to revoke";
  }

  return corrections;
}

std::vector<ResourceUsage_Executor>
CacheOccupancyStrategy::getCmtEnabledExecutors(
    const std::list<ResourceUsage_Executor>& _executors) const {
  std::vector<ResourceUsage_Executor> executors;
#ifdef CMT_ENABLED
  for (const ResourceUsage_Executor& executor : _executors) {
    if (executor.has_statistics() &&
        executor.statistics().has_perf() &&
        executor.statistics().perf().has_llc_occupancy()) {
      executors.push_back(executor);
    }
  }
#endif
  return executors;
}

double_t CacheOccupancyStrategy::countMeanCacheOccupancy(
    const std::vector<ResourceUsage_Executor>& _executors) const {
  double_t cacheOccupacySum = 0.0;
#ifdef CMT_ENABLED
  for (const ResourceUsage_Executor& executor : _executors) {
    cacheOccupacySum += executor.statistics().perf().llc_occupancy();
  }
#endif
  return cacheOccupacySum / _executors.size();
}

std::vector<ResourceUsage_Executor>
CacheOccupancyStrategy::getExecutorsAboveMinimalAndMeanOccupancy(
    const std::vector<ResourceUsage_Executor>& _executors,
    const double_t _cacheOccupancyMean) const {
  std::vector<ResourceUsage_Executor> product;
#ifdef CMT_ENABLED
  for (const ResourceUsage_Executor& executor : _executors) {
    uint64_t cacheOccupancy = executor.statistics().perf().llc_occupancy();
    if (cacheOccupancy >= _cacheOccupancyMean &&
        cacheOccupancy > this->minimalCacheOccupancy) {
      product.push_back(executor);
    }
  }
#endif
  return product;
}

}  // namespace serenity
}  // namespace mesos
