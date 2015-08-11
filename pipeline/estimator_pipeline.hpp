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

#include "time_series_export/slack_ts_export.hpp"

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
 *            |         \
 *       |Resources|   [Resources]
 *            |                |
 *     {{ PIPELINE SINK }}  {{ Slack Time Series Export }}
 *
 * For detailed schema please see: docs/pipeline.md
 */
class CpuEstimatorPipeline : public ResourceEstimatorPipeline {
 public:
  explicit CpuEstimatorPipeline(bool _visualisation = true,
                                bool _valveOpened = true) :
      // Time series exporters.
      slackTimeSeriesExporter(),
      // Last item in pipeline.
      slackObserver(this),
      // 4th item in pipeline.
      ignoreNewExecutorsFilter(&slackObserver),
      // 3rd item in pipeline.
      prExecutorPassFilter(&ignoreNewExecutorsFilter),
      // 2nd item in pipeline.
      utilizationFilter(&prExecutorPassFilter,
                        DEFAULT_UTILIZATION_THRESHOLD,
                        Tag(RESOURCE_ESTIMATOR, "utiliationFilter")),
      // First item in pipeline.
      valveFilter(&utilizationFilter,
                  _valveOpened,
                  Tag(RESOURCE_ESTIMATOR, "valveFilter")) {
    // NOTE(bplotka): Currently we wait one minute for testing purposes.
    // However in production env 5 minutes is a better value.
    this->ignoreNewExecutorsFilter.setThreshold(60);
    // Setup beginning producer.
    this->addConsumer(&valveFilter);
    // Setup Time Series Exports
    if (_visualisation) {
      slackObserver.addConsumer(&slackTimeSeriesExporter);
    }
  }

 private:
  // --- Filters ---
  PrExecutorPassFilter prExecutorPassFilter;
  IgnoreNewExecutorsFilter ignoreNewExecutorsFilter;
  UtilizationThresholdFilter utilizationFilter;
  ValveFilter valveFilter;

  // --- Observers ---
  SlackResourceObserver slackObserver;

  // --- Time Series Exporters ---
  SlackTimeSeriesExporter slackTimeSeriesExporter;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_ESTIMATOR_PIPELINE_HPP
