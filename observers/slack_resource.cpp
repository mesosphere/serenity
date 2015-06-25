#include <memory>
#include <vector>

#include "glog/logging.h"

#include "mesos/mesos.hpp"

#include "slack_resource.hpp"

#include "serenity/executor_set.hpp"
#include "serenity/metrics_helpers.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> SlackResourceObserver::consume(const ResourceUsage& usage) {
  std::unique_ptr<ExecutorSet> newSamples(new ExecutorSet());
  std::vector<Resource> slackResources;

  for (const auto& executor : usage.executors()) {
    if (executor.has_statistics() && executor.has_executor_info()) {
      newSamples->insert(executor);

      auto previousSample = this->previousSamples->find(executor);
      if (previousSample != this->previousSamples->end()) {
        Result<Resource> slackResource = CalculateSlack(
            (*previousSample), executor);
        if (slackResource.isSome()) {
          slackResources.push_back(slackResource.get());
        } else if (slackResource.isError()) {
          LOG(ERROR) << slackResource.error();
        }
      }
    }
  }

  if (!slackResources.empty()) {
    Result<Resource> result = CombineSlack(slackResources);
    if (result.isSome()) {
      produce(result.get());
    }
  }

  this->previousSamples->clear();
  this->previousSamples = std::move(newSamples);

  return Nothing();
}


/**
 * CPU slack resource is counted by equation
 * cpu_allocation - ((cpu_secs_used)/sampling_duration)
 */
Result<Resource> SlackResourceObserver::CalculateSlack(
    const ResourceUsage_Executor& prev,
    const ResourceUsage_Executor& current) const {

  Try<double_t> cpuUsage = CountCpuUsage(prev, current);
  if (cpuUsage.isError()) {
    return Error(cpuUsage.error());
  } else if (!current.statistics().has_cpus_limit()) {
    return Error("Cannot count slack. You lack cpus_limit in statistcs");
  }

  double_t cpuLimit = current.statistics().cpus_limit();
  double_t cpuSlack = cpuLimit - cpuUsage.get();

  if (cpuSlack < SLACK_EPSILON) {
    return None();
  } else {
    Resource result;

    Value_Scalar *cpuSlackScalar = new Value_Scalar();
    cpuSlackScalar->set_value(cpuSlack);
    Resource_RevocableInfo *revocableInfo = new Resource_RevocableInfo();

    result.set_name("cpus");
    result.set_type(Value::SCALAR);
    result.set_allocated_scalar(cpuSlackScalar);
    result.set_allocated_revocable(revocableInfo);
    return result;
  }
}

/**
 * Combine vector of slack resources into one
 */
Result<Resource> SlackResourceObserver::CombineSlack(
    const std::vector<Resource>& slackResources) const {
  double_t cpuSlack = 0;
  for (const auto& resource : slackResources) {
    cpuSlack += resource.scalar().value();
  }

  Resource slackResult;
  Value_Scalar *cpuSlackScalar = new Value_Scalar();
  cpuSlackScalar->set_value(cpuSlack);
  Resource_RevocableInfo *revocableInfo = new Resource_RevocableInfo();
  slackResult.set_name("cpus");
  slackResult.set_type(Value::SCALAR);
  slackResult.set_allocated_scalar(cpuSlackScalar);
  slackResult.set_allocated_revocable(revocableInfo);

  return slackResult;
}

}  // namespace serenity
}  // namespace mesos
