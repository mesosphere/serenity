#ifndef SERENITY_QOS_PIPELINE_HPP
#define SERENITY_QOS_PIPELINE_HPP

#include "filters/contention_detector.hpp"
#include "filters/cumulative.hpp"
#include "filters/ema.hpp"
#include "filters/executor_age.hpp"
#include "filters/pr_executor_pass.hpp"
#include "filters/utilization_threshold.hpp"
#include "filters/valve.hpp"
#include "filters/detectors/assurance.hpp"
#include "filters/detectors/utilization.hpp"

#include "messages/serenity.hpp"

#include "pipeline/pipeline.hpp"

#include "observers/qos_correction.hpp"
#include "observers/strategies/seniority.hpp"

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
    this->applyConfig(customCfg);
  }

  void initDefaults() {
    // Used sections: QoSCorrectionObserver, AssuranceDetector,
    // UtilizationDetector
    // TODO(bplotka): Move EMA conf to separate section.
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
 *   {{ Cumulative Filter }}
 *            |
 *      |ResourceUsage|
 *       /           \
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
      qoSCorrectionObserver(
          this,
          1,
          conf["QoSCorrectionObserver"],
          &ageFilter,
          new SeniorityStrategy(conf["QoSCorrectionObserver"])),
      ipcDropFilter(
          &qoSCorrectionObserver,
          usage::getEmaIpc,
          conf[ASSURANCE_DETECTOR_NAME],
          Tag(QOS_CONTROLLER, "IPC detectorFilter")),
      cpuUtilizationFilter(
          &qoSCorrectionObserver,
          usage::getCpuUsage,
          conf[UTILIZATION_DETECTOR_NAME],
          Tag(QOS_CONTROLLER, "CPU-Usage detectorFilter")),
      emaFilter(
          &ipcDropFilter,
          usage::getIpc,
          usage::setEmaIpc,
          conf.getD(ema::ALPHA),
          Tag(QOS_CONTROLLER, "emaFilter")),
      cumulativeFilter(
        &emaFilter,
        Tag(QOS_CONTROLLER, "cumulativeFilter")),
      // First item in pipeline. For now, close the pipeline for QoS.
      valveFilter(
          &cumulativeFilter,
          conf.getB(VALVE_OPENED),
          Tag(QOS_CONTROLLER, "valveFilter")) {
    this->ageFilter.addConsumer(&valveFilter);
    // Setup starting producer.
    this->addConsumer(&ageFilter);

    // QoSCorrection needs ResourceUsage as well.
    cumulativeFilter.addConsumer(&qoSCorrectionObserver);

    emaFilter.addConsumer(&cpuUtilizationFilter);

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
  CumulativeFilter cumulativeFilter;
  EMAFilter emaFilter;
  ContentionDetectorFilter ipcDropFilter;
  ContentionDetectorFilter cpuUtilizationFilter;
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
