#include <memory>
#include <string>

#include "glog/logging.h"

#include "filters/utilization_threshold.hpp"

#include "mesos/resources.hpp"

#include "serenity/metrics_helper.hpp"

namespace mesos {
namespace serenity {

using std::string;

Try<Nothing> UtilizationThresholdFilter::consume(const ResourceUsage& product) {
  std::unique_ptr<ExecutorSet> newSamples(new ExecutorSet());
  double_t totalCpuUsage = 0;

  for (const ResourceUsage_Executor& inExec : product.executors()) {
    // In case of lack of the usage for given executor or lack of
    // executor_info filter assumes that it uses maximum of allowed
    // resource (allocated).
    bool useAllocatedForUtilization = false;
    string executor_id = "<unknown>";

    if (!inExec.has_executor_info()) {
      SERENITY_LOG(WARNING) << "Executor " << executor_id
      << " does not include executor_info. "
      << UTILIZATION_THRESHOLD_FILTER_WARNING;
      // We can work without that - using allocated.
      useAllocatedForUtilization = true;
    } else {
      executor_id = inExec.executor_info().executor_id().value();
    }

    if (!inExec.has_statistics()) {
      SERENITY_LOG(WARNING) << "Executor " << executor_id
      << " does not include statistics. "
      << UTILIZATION_THRESHOLD_FILTER_WARNING;
      // We can work without that - using allocated resources.
      useAllocatedForUtilization = true;
    }

    if (inExec.allocated().size() == 0) {
      SERENITY_LOG(ERROR) << "Executor "  << executor_id
      << " does not include allocated resources. "
      << UTILIZATION_THRESHOLD_FILTER_ERROR;
      // Filter out these executors(!)
      continue;
    }


    if (inExec.has_executor_info() && inExec.has_statistics()) {
      newSamples->insert(inExec);
      auto previousSample = this->previousSamples->find(inExec);
      if (previousSample != this->previousSamples->end()) {
        Try<double_t> cpuUsage = CountCpuUsage(
            (*previousSample), inExec);

        if (cpuUsage.isError()) {
          LOG(ERROR) << cpuUsage.error();
          useAllocatedForUtilization = true;
        } else {
          // Count CPU Usage properly.
          totalCpuUsage += cpuUsage.get();
        }

      } else {
        // In case of new executor filter assumes that it uses
        // maximum of allowed resource.
        useAllocatedForUtilization = true;
      }
    }

    if (useAllocatedForUtilization) {
      Resources allocated(inExec.allocated());
      Option<double_t> allocatedCpus = allocated.cpus();
      if (allocatedCpus.isSome())
        totalCpuUsage += allocatedCpus.get();
    }
  }

  this->previousSamples->clear();
  this->previousSamples = std::move(newSamples);

  if (this->previousSamples->empty()) {
    SERENITY_LOG(INFO)
      << "There is no Executor in given usage. Ending the pipeline.";
    return Nothing();
  }

  Resources totalSlaveResources(product.total());
  Option<double_t> totalSlaveCpus = totalSlaveResources.cpus();
  if (totalSlaveCpus.isSome() && this->previousSamples->size() > 0) {
    // Send only when node utilization is not too high.
    if ((totalCpuUsage / totalSlaveCpus.get()) < this->utilizationThreshold) {
      // Continue pipeline.
      SERENITY_LOG(INFO) << "Continuing with "
                         << product.executors_size() << " executor(s).";
      produce(product);
    } else {
      SERENITY_LOG(ERROR) << "Stopping the oversubscription - load "
          "is too high." << "CpuUsage: " << totalCpuUsage <<
          ", Slave capacity:" << totalSlaveCpus.get();
    }
  } else {
    SERENITY_LOG(WARNING) << "Does not have sufficient data to process;";
  }

  return Nothing();
}


}  // namespace serenity
}  // namespace mesos
