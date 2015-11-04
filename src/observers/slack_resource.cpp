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

  Resources totalAgentResources(usage.total());
  Option<double_t> totalAgentCpus = totalAgentResources.cpus();

  if (totalAgentCpus.isNone()) {
    return Error(std::string(NAME) +
                 "Cannot estimate slack resources. " +
                 "ResourceUsage does not contain " +
                 "Agent's total CPU resource information.");
  }

  for (const auto& executor : usage.executors()) {
    if (executor.has_statistics() && executor.has_executor_info()) {
      newSamples->insert(executor);

      auto previousSample = this->previousSamples->find(executor);
      if (previousSample != this->previousSamples->end()) {
        Try<double_t> executorCpuUsage = CountCpuUsage(
            *previousSample, executor);

        if (executorCpuUsage.isError()) {
          LOG(ERROR) << std::string(NAME) << ": " << executorCpuUsage.error();
          break;
        }
        cpuUsage += executorCpuUsage.get();

        if (!executor.statistics().has_cpus_limit()) {
          return Error(std::string(NAME) +
                       "Cannot count slack. Lack of cpus_limit in statistcs");
        }

        double_t executorCpuLimit = executor.statistics().cpus_limit();
        double_t executorCpuSlack = executorCpuLimit - executorCpuUsage.get();

        slackResources += executorCpuSlack;
      }
    }
  }


  const double_t maxSlack =
      (maxOversubscriptionFraction * totalAgentCpus.get()) - cpuUsage;
  if (maxSlack < slackResources) {
    slackResources = maxSlack;
  } else if (slackResources < SLACK_EPSILON) {
    slackResources = 0.0;
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
}


}  // namespace serenity
}  // namespace mesos
