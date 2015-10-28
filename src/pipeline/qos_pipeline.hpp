#ifndef SERENITY_QOS_PIPELINE_HPP
#define SERENITY_QOS_PIPELINE_HPP

#include "filters/detector.hpp"
#include "filters/ema.hpp"
#include "filters/executor_age.hpp"
#include "filters/pr_executor_pass.hpp"
#include "filters/utilization_threshold.hpp"
#include "filters/valve.hpp"

#include "messages/serenity.hpp"

#include "pipeline/pipeline.hpp"

#include "observers/qos_correction.hpp"

#include "serenity/config.hpp"
#include "serenity/data_utils.hpp"
#include "serenity/serenity.hpp"

#include "time_series_export/resource_usage_ts_export.hpp"

namespace mesos {
namespace serenity {

using namespace qos_pipeline;  // NOLINT(build/namespaces)

class QoSPipelineConfig : public SerenityConfig {
 public:
  QoSPipelineConfig() {}

  explicit QoSPipelineConfig(const SerenityConfig& customCfg) {
    this->initDefaults();
    this->cpy(customCfg);
  }

  void initDefaults() {
    this->sections[DetectorFilter::NAME] =
      std::make_shared<SerenityConfig>(AssuranceDetectorConfig());
    // TODO(bplotka): Moved EMA conf to separate section.
    this->fields[ema::ALPHA] = ema::DEFAULT_ALPHA;
    this->fields[VALVE_OPENED] = DEFAULT_VALVE_OPENED;
    this->fields[ENABLED_VISUALISATION] = DEFAULT_ENABLED_VISUALISATION;
  }
};


using QoSControllerPipeline = Pipeline<ResourceUsage, QoSCorrections>;


/**
 * Pipeline which includes necessary filters for making  QoS Corrections
 * based on CPU contentions.
 *   {{ PIPELINE SOURCE }}
 *            |           \
 *      |ResourceUsage|  |ResourceUsage| - {{ Raw Resource Usage Export }}
 *            |
 *       {{ Record Executor Age }} 
 *            |
 *       {{ Valve }} (+http endpoint) // First item.
 *            |
 *      |ResourceUsage|
 *       /          \
 *       |  {{ OnlyPRTaskFilter }}
 *       |           |
 *       |     |ResourceUsage|
 *       |           |
 *       |    {{ IPC EMA Filter }}
 *       |           |          \
 *       |     |ResourceUsage|  |ResourceUsage| - {{EMA Resource Usage Export}}
 *       |           |
 *       |     {{ IPC Drop<ChangePointDetector> }}
 *       |           |
 *       |      |Contentions|
 *       |           |
 * {{ QoS Correction Observer }} // Last item.
 *            |
 *      |Corrections|
 *            |
 *    {{ PIPELINE SINK }}
 *
 * For detailed schema please see: docs/pipeline.md
 */
class CpuQoSPipeline : public QoSControllerPipeline {
 public:
  explicit CpuQoSPipeline(const SerenityConfig& _conf)
    : conf(QoSPipelineConfig(_conf)),
      // Time series exporters.
      rawResourcesExporter("raw"),
      emaFilteredResourcesExporter("ema"),
      // NOTE(bplotka): age Filter should initialized first before passing
      // to the qosCorrectionObserver.
      ageFilter(),
      // Last item in pipeline.
      qoSCorrectionObserver(this, 1, &ageFilter,
                            new SeverityBasedSeniorityDecider),
      ipcDropFilter(
          &qoSCorrectionObserver,
          usage::getEmaIpc,
          conf[DetectorFilter::NAME],
          Tag(QOS_CONTROLLER, "IPC detectorFilter")),
      emaFilter(
          &ipcDropFilter,
          usage::getIpc,
          usage::setEmaIpc,
          conf.getD(ema::ALPHA),
          Tag(QOS_CONTROLLER, "emaFilter")),
      prExecutorPassFilter(&emaFilter),
      // First item in pipeline. For now, close the pipeline for QoS.
      valveFilter(
          &prExecutorPassFilter,
          conf.getB(VALVE_OPENED),
          Tag(QOS_CONTROLLER, "valveFilter")) {
    this->ageFilter.addConsumer(&valveFilter);
    // Setup starting producer.
    this->addConsumer(&ageFilter);

    // QoSCorrection needs ResourceUsage as well.
    valveFilter.addConsumer(&qoSCorrectionObserver);

    // Setup Time Series export
    if (conf.getB(ENABLED_VISUALISATION)) {
      this->addConsumer(&rawResourcesExporter);
      emaFilter.addConsumer(&emaFilteredResourcesExporter);
    }
  }

  virtual Try<Nothing> resetSyncConsumers() {
    this->qoSCorrectionObserver.reset();

    return Nothing();
  }

 private:
  SerenityConfig conf;
  // --- Filters ---
  ExecutorAgeFilter ageFilter;
  EMAFilter emaFilter;
  DetectorFilter ipcDropFilter;
  PrExecutorPassFilter prExecutorPassFilter;
  ValveFilter valveFilter;

  // --- Observers ---
  QoSCorrectionObserver qoSCorrectionObserver;

  // --- Time Series Exporters ---
  ResourceUsageTimeSeriesExporter rawResourcesExporter;
  ResourceUsageTimeSeriesExporter emaFilteredResourcesExporter;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_QOS_PIPELINE_HPP
