#include <cstdlib>
#include <ctime>
#include <string>

#include "backend/visualisation_backend.hpp"

#include "glog/logging.h"

#include "slack_visualisation.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> SlackVisualisationFilter::consume(const Resources& resources) {
  double_t cpus = 0.0;

  Option<double_t> cpus_option = resources.cpus();
  if (cpus_option.isSome()) {
    cpus = cpus_option.get();
  }

  Try<std::string> hostname = GetHostName();
  if (hostname.isError()) {
    return Error(hostname.error());
  }

  // TODO(skonefal): Consider higher resolution time provider.
  time_t timeNow = time(nullptr);

  VisualisationRecord record(Series::SLACK_RESOURCES, timeNow);
  record.setTag(Tag::VALUE, cpus);
  record.setTag(Tag::NODE, hostname.get());

  visualisationBackend->PutMetric(record);

  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
