#ifndef SERENITY_RESOURCE_USAGE_TIME_SERIES_EXPORTER_HPP
#define SERENITY_RESOURCE_USAGE_TIME_SERIES_EXPORTER_HPP

#include <string>

#include "backend/time_series_backend.hpp"
#include "backend/influx_db8.hpp"

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

/**
 * Time series exporter for ResourceUsage message.
 *
 * @param _timeSeriesBackend: Time Series Backend.
 * @param _tag: Custom tag added to every sample.
 */
class ResourceUsageTimeSeriesExporter : public Consumer<ResourceUsage> {
 public:
  ResourceUsageTimeSeriesExporter(
      Variant _tag = "",
      TimeSeriesBackend* _timeSeriesBackend = new InfluxDb8Backend()) :
        timeSeriesBackend(_timeSeriesBackend),
        customTag(_tag) {}

  Try<Nothing> consume(const ResourceUsage& resources) override;

 protected:
  TimeSeriesBackend* timeSeriesBackend;

  std::string hostname;

  const Variant customTag;  //!< Custom tag that is added to every sample.
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_RESOURCE_USAGE_TIME_SERIES_EXPORTER_HPP
