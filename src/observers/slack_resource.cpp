#include <memory>
#include <string>
#include <vector>

#include "glog/logging.h"

#include "mesos/mesos.hpp"

#include "observers/slack_resource.hpp"

#include "serenity/metrics_helper.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> SlackResourceObserver::consume(const ResourceUsage& usage) {
  std::unique_ptr<ExecutorSet> newSamples(new ExecutorSet());
  double_t cpuUsage = 0;
  double_t slackResources = 0;

  for (const auto& executor : usage.executors()) {
    if (executor.has_statistics() && executor.has_executor_info()) {
      newSamples->insert(executor);

      auto previousSample = this->previousSamples->find(executor);
      if (previousSample != this->previousSamples->end()) {
        Result<double_t> executorCpuSlack = CalculateCpuSlack(
            *previousSample, executor);
        Try<double_t> executorCpuUsage = CountCpuUsage(
            *previousSample, executor);
        if (executorCpuSlack.isSome()) {
          slackResources += executorCpuSlack.get();
        } else if (executorCpuSlack.isError()) {
          LOG(ERROR) << executorCpuSlack.error();
        }

        if (executorCpuUsage.isSome()) {
          cpuUsage += executorCpuUsage.get();
        } else if (executorCpuUsage.isError()) {
          LOG(ERROR) << executorCpuSlack.error();
        }
      }
    }
  }

  Resources totalAgentResources(usage.total());
  Option<double_t> totalAgentCpus = totalAgentResources.cpus();

  if (totalAgentCpus.isSome()) {
    const double_t maxSlack =
        (maxOversubscriptionFraction * totalAgentCpus.get()) - cpuUsage;
    if (maxSlack < slackResources) {
      slackResources = maxSlack;
    }

    Resource slackResult;
    Value_Scalar *cpuSlackScalar = new Value_Scalar();
    cpuSlackScalar->set_value(slackResources);

    slackResult.set_name("cpus");
    slackResult.set_role(this->default_role);
    slackResult.set_type(Value::SCALAR);
    slackResult.set_allocated_scalar(cpuSlackScalar);
    slackResult.set_allocated_revocable(new Resource_RevocableInfo());

    Resources result(slackResult);

    produce(result);

    this->previousSamples->clear();
    this->previousSamples = std::move(newSamples);

    return Nothing();
  } else {
    return Error(std::string(NAME) +
                 "Cannot estimate slack resources. " +
                 "ResourceUsage does not contain " +
                 "Agent's total resource information.");
  }
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
    return Error(std::string(NAME) +
                 "Cannot count slack. Lack of cpus_limit in statistcs");
  }

  /// cpuLimit - executor allocation
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
