#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#include "backend/visualisation_backend.hpp"

#include "glog/logging.h"

#include "resource_usage_visualisation.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> ResourceUsageVisualisation::consume(const ResourceUsage& _res) {
  std::vector<VisualisationRecord> product;
  Try<std::string> hostname = GetHostName();
  if (hostname.isError()) {
    return Error(hostname.error());
  }

  for (const auto& executor : _res.executors()) {
    if (!executor.has_executor_info() || !executor.has_statistics()) {
      LOG(WARNING) << "ResourceUsageVisualisation: "
                   << "Executor does not have required fields";
      continue;
    }

    const auto& info = executor.executor_info();
    const auto& stats = executor.statistics();
    const double_t sampleTime = stats.timestamp();

    std::vector<VisualisationRecord> locals;
    locals.push_back(VisualisationRecord(Series::CPU_USAGE_SYS,
                                         sampleTime,
                                         stats.cpus_system_time_secs()));
    locals.push_back(VisualisationRecord(Series::CPU_USAGE_USR,
                                         sampleTime,
                                         stats.cpus_user_time_secs()));
    locals.push_back(VisualisationRecord(Series::CPU_ALLOC,
                                         sampleTime,
                                         stats.cpus_limit()));

    for (auto& local : locals) {
      local.setTag(Tag::FRAMEWORK_ID, info.framework_id().value());
      local.setTag(Tag::EXECUTOR_ID, info.framework_id().value());
      local.setTag(Tag::NODE, hostname.get());
      local.setTag(Tag::NODE, hostname.get());
      local.setTag(Tag::TAG, this->customTag);

      product.push_back(local);
    }
  }

  this->visualisationBackend->PutMetric(product);

  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
