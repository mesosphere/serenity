#include <string>

#include "glog/logging.h"

#include "filters/utilization_threshold.hpp"

#include "serenity/metrics_helper.hpp"

namespace mesos {
namespace serenity {

using std::string;

const string UTILIZATION_THRESHOLD_FILTER_ERROR = "Filter is not able to" \
" calculate total cpu usage and cut off oversubscription if needed.";

const string UTILIZATION_THRESHOLD_FILTER_WARNING = "Filter is not able to" \
             " calculate total cpu usage and will base on allocated " \
             " resources to cut off oversubscription if needed.";


Try<Nothing> UtilizationThresholdFilter::consume(const ResourceUsage& in) {
  std::unique_ptr<ExecutorSet> newSamples(new ExecutorSet());
  double_t totalCpuUsage = 0;

  for (const auto& executor : usage.executors()) {
    // In case of lack of the usage for given executor or lack of
    // executor_info filter assumes that it uses maximum of allowed
    // resource (allocated).
    bool useAllocatedForUtilization = false;
    string executor_id = "<unknown>";

    if (!inExec.has_executor_info()) {
      LOG(WARNING) << "Executor " << executor_id
      << " does not include executor_info. "
      << UTILIZATION_THRESHOLD_FILTER_WARNING;
      // We can work without that - using allocated.
      useAllocatedForUtilization = true;
    }
    else {
      executor_id = inExec.executor_info().executor_id().value();
    }

    if (!inExec.has_statistics()) {
      LOG(WARNING) << "Executor " << executor_id
      << " does not include statistics. "
      << UTILIZATION_THRESHOLD_FILTER_WARNING;
      // We can work without that - using allocated resources.
      useAllocatedForUtilization = true;
    }

    if (!inExec.allocated().size() == 0) {
      LOG(ERROR) << "Executor "  << executor_id
      << " does not include allocated resources. "
      << UTILIZATION_THRESHOLD_FILTER_ERROR;
      // Filter out these executors(!)
      continue;
    }

    if (inExec.has_executor_info() && inExec.has_statistics()) {
      newSamples->insert(inExec);
      auto previousSample = this->previousSamples->find(executor);
      if (previousSample != this->previousSamples->end()) {
        Try<double_t> cpuUsage = CountCpuUsage(
            (*previousSample), executor);

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
      totalCpuUsage += allocated.get("cpus");
    }
  }

  this->previousSamples->clear();
  this->previousSamples = std::move(newSamples);

  // Send only when node utilization is not too high.
  if ((totalCpuUsage + this->utilizationThreshold) < in.total())
    produce(in);

  return Nothing();
}


}  // namespace serenity
}  // namespace mesos
