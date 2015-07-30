#ifndef SERENITY_SLACK_VISUALISATION_FILTER_HPP
#define SERENITY_SLACK_VISUALISATION_FILTER_HPP

#include "backend/visualisation_backend.hpp"
#include "backend/influx_db8.hpp"

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

class SlackVisualisationFilter : public Consumer<Resources> {
 public:
  SlackVisualisationFilter() {
    visualisationBackend = new InfluxDb8Backend();
  }

  Try<Nothing> consume(const Resources& resources) override;

 protected:
  VisualisationBackend* visualisationBackend;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_SLACK_VISUALISATION_FILTER_HPP
