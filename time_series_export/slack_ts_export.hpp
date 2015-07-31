#ifndef SERENITY_SLACK_TIME_SERIES_EXPORTER_HPP
#define SERENITY_SLACK_TIME_SERIES_EXPORTER_HPP

#include "backend/time_series_backend.hpp"
#include "backend/influx_db8.hpp"

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

class SlackTimeSeriesExporter : public Consumer<Resources> {
 public:
  SlackTimeSeriesExporter(
      TimeSeriesBackend* _timeSeriesBackend = new InfluxDb8Backend(),
      Variant _tag = "") :
  timeSeriesBackend(_timeSeriesBackend),
  customTag(_tag) {}

  Try<Nothing> consume(const Resources& resources) override;

 protected:
  TimeSeriesBackend* timeSeriesBackend;

  const Variant customTag;  //!< Custom tag that is added to every sample.
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_SLACK_TIME_SERIES_EXPORTER_HPP
