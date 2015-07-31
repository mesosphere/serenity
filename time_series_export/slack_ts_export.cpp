#include <string>

#include "glog/logging.h"

#include "serenity/agent_utils.hpp"

#include "slack_ts_export.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> SlackTimeSeriesExporter::consume(const Resources& resources) {
  double_t cpus = 0.0;

  Option<double_t> cpus_option = resources.cpus();
  if (cpus_option.isSome()) {
    cpus = cpus_option.get();
  }

  Try<std::string> hostname = AgentInfo::GetHostName();
  if (hostname.isError()) {
    return Error(hostname.error());
  }
  Try<std::string> agentId = AgentInfo::GetAgentId();
  if (agentId.isError()) {
    return Error(agentId.error());
  }

  TimeSeriesRecord record(Series::SLACK_RESOURCES);
  record.setTag(Tag::VALUE, cpus);
  record.setTag(Tag::HOSTNAME, hostname.get());
  record.setTag(Tag::AGENT_ID, agentId.get());
  record.setTag(Tag::TAG, this->customTag);

  timeSeriesBackend->PutMetric(record);

  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
