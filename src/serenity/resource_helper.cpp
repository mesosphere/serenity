#include <list>
#include <tuple>

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "serenity/resource_helper.hpp"

namespace mesos {
namespace serenity {

std::list<ResourceUsage_Executor>
ResourceUsageHelper::getRevocableExecutors(
    const ResourceUsage& usage) {
  auto executorListsTuple = getProductionAndRevocableExecutors(usage);
  return std::get<ExecutorType::REVOCABLE>(executorListsTuple);
}

std::list<ResourceUsage_Executor>
ResourceUsageHelper::getProductionExecutors(
    const ResourceUsage& usage) {
  auto executorListsTuple = getProductionAndRevocableExecutors(usage);
  return std::get<ExecutorType::PRODUCTION>(executorListsTuple);
}

std::tuple<
  std::list<ResourceUsage_Executor>,
  std::list<ResourceUsage_Executor>>
ResourceUsageHelper::getProductionAndRevocableExecutors(
    const ResourceUsage& usage) {
  std::list<ResourceUsage_Executor> productionExecutors;
  std::list<ResourceUsage_Executor> revocableExecutors;
  for (ResourceUsage_Executor executor : usage.executors()) {
    Resources allocated(executor.allocated());

    if (allocated.revocable().empty()) {
      productionExecutors.push_back(executor);
    } else {
      revocableExecutors.push_back(executor);
    }
  }
  return std::make_tuple(productionExecutors,
                         revocableExecutors);
}

bool ResourceUsageHelper::isProductionExecutor(
const ResourceUsage_Executor& executor) {
  return Resources(executor.allocated()).revocable().empty();
}

bool ResourceUsageHelper::isExecutorHasStatistics(
    const ResourceUsage_Executor& executor) {
  return executor.has_executor_info() && executor.has_statistics();
}

bool ResourceUsageHelper::isRevocableExecutor(
    const ResourceUsage_Executor &executor) {
  return !isProductionExecutor(executor);
}

}  // namespace serenity
}  // namespace mesos
