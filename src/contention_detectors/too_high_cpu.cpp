#include <string>
#include <utility>

#include "contention_detectors/too_high_cpu.hpp"

#include "mesos/resources.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> TooHighCpuUsageDetector::consume(const ResourceUsage& in) {
  Contentions product;
  SERENITY_LOG(INFO) << "debug1";
  if (in.total_size() == 0) {
    return Error(std::string(NAME) + " No total in ResourceUsage");
  }

  Resources totalAgentResources(in.total());
  Option<double_t> totalAgentCpus = totalAgentResources.cpus();

  if (totalAgentCpus.isNone()) {
    return Error(std::string(NAME) + " No total cpus in ResourceUsage");
  }

  double_t agentSumValue = 0;

  for (const ResourceUsage_Executor& inExec : in.executors()) {
    if (!inExec.has_executor_info()) {
      SERENITY_LOG(ERROR) << "Executor <unknown>"
      << " does not include executor_info";
      // Filter out these executors.
      continue;
    }
    if (!inExec.has_statistics()) {
      SERENITY_LOG(ERROR) << "Executor "
      << inExec.executor_info().executor_id().value()
      << " does not include statistics.";
      // Filter out these executors.
      continue;
    }

    SERENITY_LOG(INFO) << "Getting value...";
    Try<double_t> value = this->cpuUsageGetFunction(inExec);
    if (value.isError()) {
      SERENITY_LOG(ERROR) << value.error();
      continue;
    }
    SERENITY_LOG(INFO) << "Got value: " << value.get();
    agentSumValue += value.get();
  }

  // Debug only
  SERENITY_LOG(INFO) << "Sum = " << agentSumValue << " vs total = "
                     << totalAgentCpus.get();
  double_t lvl = agentSumValue / totalAgentCpus.get();

  if (lvl > this->cfgUtilizationThreshold) {
    SERENITY_LOG(INFO) << "Creating REVOKE ALL contention, because of the value"
      " above the threshold. " << agentSumValue << "/" << totalAgentCpus.get();
    product.push_back(createCpuContention(KILL_ALL_SEVERITY));
  }

  // Continue pipeline.
  this->produce(product);

  return Nothing();
}


}  // namespace serenity
}  // namespace mesos
