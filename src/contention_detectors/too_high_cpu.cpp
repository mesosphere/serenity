#include <string>
#include <utility>

#include "contention_detectors/too_high_cpu.hpp"

#include "mesos/resources.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> TooHighCpuUsageDetector::consume(const ResourceUsage& in) {
  Contentions product;

  if (in.total_size() == 0) {
    return Error(std::string(NAME) + " No total in ResourceUsage");
  }

  Resources totalAgentResources(in.total());
  Option<double_t> totalAgentCpus = totalAgentResources.cpus();

  if (totalAgentCpus.isNone()) {
    return Error(std::string(NAME) + " No total cpus in ResourceUsage");
  }

  double_t thresholdCpus = this->cfgUtilizationThreshold * totalAgentCpus.get();
  double_t agentSumCpus = 0;
  uint64_t beExecutors = 0;

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

    Try<double_t> value = this->cpuUsageGetFunction(inExec);
    if (value.isError()) {
      SERENITY_LOG(ERROR) << value.error();
      continue;
    }

    agentSumCpus += value.get();

    if (!Resources(inExec.allocated()).revocable().empty()) {
      beExecutors++;
    }
  }

  SERENITY_LOG(INFO) << "Sum = " << agentSumCpus << " vs total = "
    << totalAgentCpus.get() << " [threshold = " << thresholdCpus << "]";

  if (agentSumCpus > thresholdCpus) {
    if (beExecutors == 0) {
      SERENITY_LOG(INFO) << "No BE tasks - only high host utilization";
    } else {
      // Severity is the amount of the CPUs above the threshold.
      double_t severity = agentSumCpus - thresholdCpus;
      SERENITY_LOG(INFO) << "Creating CPU contention, because of the "
                         << severity << " CPUs above the threshold. ";

      product.push_back(createContention(severity, Contention_Type_CPU));
    }
  }

  // Continue pipeline.
  this->produce(product);

  return Nothing();
}


}  // namespace serenity
}  // namespace mesos
