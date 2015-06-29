#include <memory>
#include <vector>

#include "glog/logging.h"

#include "mesos/mesos.hpp"

#include "observers/slack_resource.hpp"

#include "serenity/metrics_helper.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> SlackResourceObserver::consume(const ResourceUsage& usage) {
  std::unique_ptr<ExecutorSet> newSamples(new ExecutorSet());
  double_t cpuSlack = 0;

  for (const auto& executor : usage.executors()) {
    if (executor.has_statistics() && executor.has_executor_info()) {
      newSamples->insert(executor);

      auto previousSample = this->previousSamples->find(executor);
      if (previousSample != this->previousSamples->end()) {
        Result<double_t> slackResource = CalculateCpuSlack(
            (*previousSample), executor);
        if (slackResource.isSome()) {
            cpuSlack += slackResource.get();
        } else if (slackResource.isError()) {
          LOG(ERROR) << slackResource.error();
        }
      }
    }
  }

  Resource slackResult;
  Value_Scalar *cpuSlackScalar = new Value_Scalar();
  cpuSlackScalar->set_value(cpuSlack);

  slackResult.set_name("cpus");
  slackResult.set_type(Value::SCALAR);
  slackResult.set_allocated_scalar(cpuSlackScalar);
  slackResult.set_allocated_revocable(new Resource_RevocableInfo());

  Resources result(slackResult);

  produce(result);

  this->previousSamples->clear();
  this->previousSamples = std::move(newSamples);

  return Nothing();
}


/**
 * CPU slack resource is counted by equation
 * cpu_allocation - (cpu_secs_used / sampling_duration)
 */
Result<double_t> SlackResourceObserver::CalculateCpuSlack(
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
    return cpuSlack;
  }
}

}  // namespace serenity
}  // namespace mesos
