#ifndef SERENITY_ESTIMATOR_PIPELINE_HPP
#define SERENITY_ESTIMATOR_PIPELINE_HPP

#include "filters/ignore_new_executors.hpp"
#include "filters/pr_executor_pass.hpp"
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
 *   {{ PIPELINE SOURCE }}
 *            |
 *      |ResourceUsage|
 *            |
 *       {{ Valve }} (+http endpoint) // First item.
 *            |
 *      |ResourceUsage|
 *            |
 * {{ Utilization Filter }}  // 2nd item.
 *            |
 *      |ResourceUsage|
 *            |
 *  {{ PR Executors Pass }}   //3rd item.
 *            |
 *      |ResourceUsage|
 *            |
 *  {{ Ignore New Executors }} //4th item.
 *            |
 *      |ResourceUsage|
 *            |
 *    {{ Slack Observer }} // Last item.
 *            |
 *       |Resources|
 *            |
 *     {{ PIPELINE SINK }}
 *
 * For detailed schema please see: docs/pipeline.md
 */
class CpuEstimatorPipeline : public ResourceEstimatorPipeline {
 public:
  CpuEstimatorPipeline() :
      // Last item in pipeline.
      slackObserver(this),
      // 4th item in pipeline.
      ignoreNewExecutorsFilter(&slackObserver),
      // 3rd item in pipeline.
      prExecutorPassFilter(&ignoreNewExecutorsFilter),
      // 2nd item in pipeline.
      utilizationFilter(&prExecutorPassFilter, DEFAULT_UTILIZATION_THRESHOLD),
      // First item in pipeline.
      valveFilter(ValveFilter(
          &utilizationFilter, ValveType::RESOURCE_ESTIMATOR_VALVE)) {
    // Setup beginning producer.
    this->ignoreNewExecutorsFilter.setThreshold(60);  // One minute.
    this->addConsumer(&valveFilter);
  }

 private:
  // --- Filters ---
  PrExecutorPassFilter prExecutorPassFilter;
  IgnoreNewExecutorsFilter ignoreNewExecutorsFilter;
  UtilizationThresholdFilter utilizationFilter;
  ValveFilter valveFilter;

  // --- Observers ---
  SlackResourceObserver slackObserver;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_ESTIMATOR_PIPELINE_HPP
