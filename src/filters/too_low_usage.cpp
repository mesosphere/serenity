#include <atomic>
#include <string>
#include <utility>

#include "glog/logging.h"

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "filters/too_low_usage.hpp"

#include "serenity/data_utils.hpp"

namespace mesos {
namespace serenity {

using std::map;
using std::pair;
using std::string;


const constexpr char* TooLowUsageFilter::MINIMAL_CPU_USAGE_KEY;

TooLowUsageFilter::~TooLowUsageFilter() {}


Try<Nothing> TooLowUsageFilter::consume(const ResourceUsage& in) {
  ResourceUsage product;
  product.mutable_total()->CopyFrom(in.total());

  for (const ResourceUsage_Executor& inExec : in.executors()) {
    string executor_id = "<unknown>";

    if (!inExec.has_executor_info()) {
      SERENITY_LOG(ERROR) << "Executor " << executor_id
      << " does not include executor_info. ";
      // Filter out these executors.
      continue;
    } else {
      executor_id = inExec.executor_info().executor_id().value();
    }

    if (!inExec.has_statistics()) {
      SERENITY_LOG(ERROR) << "Executor " << executor_id
      << " does not include statistics. ";
      // Filter out these executors.
      continue;
    }

    Resources allocated(inExec.allocated());
    // Check if task uses revocable resources.
    if (allocated.revocable().empty()) {
      // Consider this executor as PR.
      // Check if CPU Usage is not too low.
      // (Signal is jitter when CPU is too low)
      // NOTE(bplotka): We pick non-ema CPU Usage here to have the freshest
      // data.
      Try<double_t> cpuUsage = usage::getCpuUsage(inExec);
      if (cpuUsage.isError()) {
        SERENITY_LOG(ERROR) << cpuUsage.error();
        continue;
      }

      if (cpuUsage.get() <= cfgMinimalCpuUsage) {
        // Exclude executor.
        SERENITY_LOG(INFO) << "Filtering out PR exec: " << executor_id
          << " because of its CPU Usage: " << cpuUsage.get() << " [Min: "
          << cfgMinimalCpuUsage << "]";
        continue;
      }
    }

    // Add not excluded executor.
    ResourceUsage_Executor* outExec = product.mutable_executors()->Add();
    outExec->CopyFrom(inExec);
  }

  // Continue pipeline.
  produce(product);

  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
