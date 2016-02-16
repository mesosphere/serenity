#include <string>
#include <utility>

#include "contention_detectors/overload.hpp"

#include "mesos/resources.hpp"

#include "serenity/resource_helper.hpp"

namespace mesos {
namespace serenity {

void OverloadDetector::allProductsReady() {
  Contentions product;
  ResourceUsage usage = getConsumable().get();

  if (usage.total_size() == 0) {
    SERENITY_LOG(ERROR) << std::string(NAME) << " No total in ResourceUsage";
    produce(product);
  }

  Resources totalAgentResources(usage.total());
  Option<double_t> totalAgentCpus = totalAgentResources.cpus();

  if (totalAgentCpus.isNone()) {
    SERENITY_LOG(ERROR) << std::string(NAME)
    << " No total cpus in ResourceUsage";
    produce(product);
  }

  double_t thresholdCpus = this->cfgUtilizationThreshold * totalAgentCpus.get();
  double_t agentSumCpus = 0;
  uint64_t beExecutors = 0;

  for (const ResourceUsage_Executor& inExec : usage.executors()) {
    // Validate for statistics and executor info.
    if (!validate(inExec)) {
      continue;
    }

    Try<double_t> value = this->cpuUsageGetFunction(inExec);
    if (value.isError()) {
      SERENITY_LOG(ERROR) << value.error();
      continue;
    }

    agentSumCpus += value.get();

    if (ResourceUsageHelper::isRevocableExecutor(inExec)) {
      beExecutors++;
    }
  }

  SERENITY_LOG(INFO) << "Sum = " << agentSumCpus << " vs total = "
                     << totalAgentCpus.get() << " [threshold = "
                     << thresholdCpus << "]";

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

  produce(product);
}

bool OverloadDetector::validate(const ResourceUsage_Executor& inExec) {
  if (!inExec.has_executor_info()) {
    SERENITY_LOG(ERROR) << "Executor <unknown>"
    << " does not include executor_info";
    // Filter out these executors.
    return false;
  }

  if (!inExec.has_statistics()) {
    SERENITY_LOG(ERROR) << "Executor "
    << inExec.executor_info().executor_id().value()
    << " does not include statistics.";
    // Filter out these executors.
    return false;
  }

  return true;
}

}  // namespace serenity
}  // namespace mesos
