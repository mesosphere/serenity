#ifndef SERENITY_ESTIMATOR_PIPELINE_HPP
#define SERENITY_ESTIMATOR_PIPELINE_HPP

#include "filters/utilization_threshold.hpp"
#include "filters/valve.hpp"

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "observers/slack_resource.hpp"

#include "pipeline/pipeline.hpp"

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

using ResourceEstimatorPipeline = Pipeline<ResourceUsage, Resources>;

/**
 * Pipeline which includes necessary filters for cpu estimation.
 * It includes:
 * - utilizationFilter
 * - valveFilter
 * - slackObserver
 */
class CpuEstimatorPipeline : public ResourceEstimatorPipeline {
 public:
  CpuEstimatorPipeline() :
      // Last item in pipeline.
      slackObserver(this),
      // Item before slack observer.
      valveFilter(ValveFilter(
          &slackObserver, ValveType::RESOURCE_ESTIMATOR_VALVE)),
      // First item in pipeline.
      utilizationFilter(&valveFilter, DEFAULT_UTILIZATION_THRESHOLD) {
    // Setup starting producer.
    this->addConsumer(&utilizationFilter);
  }

 private:
  // --- Filters ---
  UtilizationThresholdFilter utilizationFilter;
  ValveFilter valveFilter;

  // --- Observers ---
  SlackResourceObserver slackObserver;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_ESTIMATOR_PIPELINE_HPP
