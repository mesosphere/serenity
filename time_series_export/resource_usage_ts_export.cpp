#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#include "serenity/data_utils.hpp"

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
    LOG(ERROR) << "ResourceUsageTimeSeriesExporter: cannot get hostname";
    return Nothing();  // Do not cause failure in pipeline due to
                       // stats reporting failure
  }
  Try<std::string> agentId = AgentInfo::GetAgentId();
  if (agentId.isError()) {
    LOG(ERROR) << "ResourceUsageTimeSeriesExporter: cannot get agent id";
    return Nothing();  // Do not cause failure in pipeline due to
                       // stats reporting failure
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

    //TODO (skonefal): Make this also send CPU_USAGE_SUM without EMA
    Try<double_t> emaCpuUsage = usage::getEmaCpuUsage(executor, executor);
    if (emaCpuUsage.isSome()) {
      locals.push_back(TimeSeriesRecord(Series::CPU_USAGE_SUM,
                                        emaCpuUsage.get()));
    }

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

      if (perf.has_cycles() && perf.has_instructions()) {
        //TODO (skonefal): When filters will be fixed, send also raw-ema
        Try<double_t> emaIpc = usage::getEmaIpc(executor,executor);
        if (emaIpc.isSome()) {
          locals.push_back(TimeSeriesRecord(Series::CPI, emaIpc.get()));
        }
      }

      if (perf.has_cache_misses()) {
        locals.push_back(TimeSeriesRecord(Series::CACHE_MISSES,
                                          perf.cache_misses()));
      }
    }

    for (auto& local : locals) {
      local.setTag(TsTag::FRAMEWORK_ID, info.framework_id().value());
      local.setTag(TsTag::EXECUTOR_ID, info.framework_id().value());
      local.setTag(TsTag::HOSTNAME, hostname.get());
      local.setTag(TsTag::AGENT_ID, agentId.get());
      local.setTag(TsTag::TAG, this->customTag);

      product.push_back(local);
    }
  }

  this->timeSeriesBackend->PutMetric(product);

  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
