#ifndef SERENITY_RESOURCE_USAGE_VISUALISATION_HPP
#define SERENITY_RESOURCE_USAGE_VISUALISATION_HPP

#include "backend/visualisation_backend.hpp"
#include "backend/influx_db8.hpp"

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

/**
 * Visualisation for ResourceUsage message.
 *
 * @param _visualisationBackend: Visualisation Backend.
 * @param _tag: Custom tag added to every sample.
 */
class ResourceUsageVisualisation : public Consumer<ResourceUsage> {
 public:
  ResourceUsageVisualisation(
      IVisualisationBackend* _visualisationBackend = new InfluxDb8Backend(),
      Variant _tag = "") :
       visualisationBackend(_visualisationBackend),
       customTag(_tag) {}

  Try<Nothing> consume(const ResourceUsage& resources) override;

 protected:
  IVisualisationBackend* visualisationBackend;

  const Variant customTag;  //!< Custom tag that is added to every sample.
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_RESOURCE_USAGE_VISUALISATION_HPP
