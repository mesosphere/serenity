#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#include "glog/logging.h"

#include "serenity/agent_utils.hpp"

#include "resource_usage_ts_export.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> ResourceUsageTimeSeriesExporter::consume(
    const ResourceUsage& _res) {
  std::vector<TimeSeriesRecord> product;
  Try<std::string> hostname = AgentInfo::GetHostName();
  if (hostname.isError()) {
    return Error(hostname.error());
  }
  Try<std::string> agentId = AgentInfo::GetAgentId();
  if (agentId.isError()) {
    return Error(agentId.error());
  }

  for (const auto& executor : _res.executors()) {
    if (!executor.has_executor_info() || !executor.has_statistics()) {
      LOG(WARNING) << "ResourceUsageVisualisation: "
                   << "Executor does not have required fields";
      continue;
    }

    const auto& info = executor.executor_info();
    const auto& stats = executor.statistics();

    std::vector<TimeSeriesRecord> locals;
    locals.push_back(TimeSeriesRecord(Series::CPU_USAGE_SYS,
                                         stats.cpus_system_time_secs()));
    locals.push_back(TimeSeriesRecord(Series::CPU_USAGE_USR,
                                         stats.cpus_user_time_secs()));
    locals.push_back(TimeSeriesRecord(Series::CPU_ALLOC,
                                         stats.cpus_limit()));

    // perf stats if exists
    if (stats.has_perf()) {
      PerfStatistics perf = stats.perf();
      if (perf.has_cycles()) {
        locals.push_back(TimeSeriesRecord(Series::CYCLES,
                                          perf.cycles()));
      }
      if (perf.has_instructions()) {
        locals.push_back(TimeSeriesRecord(Series::INSTRUCTIONS,
                                          perf.instructions()));
      }
      if (perf.has_cache_misses()) {
        locals.push_back(TimeSeriesRecord(Series::CACHE_MISSES,
                                          perf.cache_misses()));
      }
    }

    for (auto& local : locals) {
      local.setTag(Tag::FRAMEWORK_ID, info.framework_id().value());
      local.setTag(Tag::EXECUTOR_ID, info.framework_id().value());
      local.setTag(Tag::HOSTNAME, hostname.get());
      local.setTag(Tag::AGENT_ID, agentId.get());
      local.setTag(Tag::TAG, this->customTag);

      product.push_back(local);
    }
  }

  this->timeSeriesBackend->PutMetric(product);

  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
