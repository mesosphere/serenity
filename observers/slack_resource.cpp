#include <memory>
#include <string>
#include <vector>

#include "mesos/mesos.hpp"

#include "slack_resource.hpp"


namespace mesos {
namespace serenity {

Try<Nothing> SlackResourceObserver::consume(const ResourceUsage& usage) {
  std::unique_ptr<std::unordered_set<ExecutorSnapshot,
      ExecutorSnapshotHasher>> newSamples(new
        std::unordered_set<ExecutorSnapshot, ExecutorSnapshotHasher>);

  std::vector<Resource> slackResources;
  for (auto itr = usage.executors().begin();
      itr != usage.executors().end();
      ++itr) {
    if (itr->has_statistics() && itr->has_executor_info()) {
      std::string executorId = itr->executor_info().executor_id().value();
      std::string frameworkId = itr->executor_info().framework_id().value();
      double_t timestamp = itr->statistics().timestamp();
      double_t cpu_limit = itr->statistics().cpus_limit();
      double_t cpus_us = itr->statistics().cpus_user_time_secs();
      double_t cpus_sy = itr->statistics().cpus_system_time_secs();
      double_t cpus_time = cpus_sy + cpus_us;

      ExecutorSnapshot currExecutor(frameworkId, executorId,
                                timestamp, cpus_time);
      newSamples->insert(currExecutor);

      auto previousSample = this->previousSamples->find(currExecutor);
      if (previousSample != this->previousSamples->end()) {
        Result<Resource> slackResource = CalculateSlack(
            (*previousSample), currExecutor, cpu_limit);
        if (slackResource.isSome()) {
          slackResources.push_back(slackResource.get());
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
 * cpu_allocation - ((cpu_secs_used * cpu_allocation)/sampling_duration)
 */
Result<Resource> SlackResourceObserver::CalculateSlack(
    const ExecutorSnapshot& prev, const ExecutorSnapshot& current,
    double_t cpuAllocation) const {
  double_t samplingDuration = current.timestamp - prev.timestamp;
  double_t cpuTimeUsage = current.cpuUsageTime - prev.cpuUsageTime;

  double_t cpuSlack =
      cpuAllocation - ((cpuTimeUsage * cpuAllocation) / samplingDuration);

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
