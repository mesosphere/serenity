#ifndef SERENITY_CPU_QOS_PIPELINE_HPP
#define SERENITY_CPU_QOS_PIPELINE_HPP

#include "contention_detectors/signal_based.hpp"
#include "contention_detectors/too_high_cpu.hpp"
#include "contention_detectors/signal_analyzers/drop.hpp"

#include "filters/correction_merger.hpp"
#include "filters/cumulative.hpp"
#include "filters/ema.hpp"
#include "filters/executor_age.hpp"
#include "filters/pr_executor_pass.hpp"
#include "filters/too_low_usage.hpp"
#include "filters/utilization_threshold.hpp"
#include "filters/valve.hpp"

#include "messages/serenity.hpp"

#include "pipeline/pipeline.hpp"

#include "observers/qos_correction.hpp"
#include "observers/strategies/seniority.hpp"
#include "observers/strategies/cpu_contention.hpp"

#include "serenity/config.hpp"
#include "serenity/data_utils.hpp"
#include "serenity/serenity.hpp"

#include "time_series_export/resource_usage_ts_export.hpp"

namespace mesos {
namespace serenity {

using namespace qos_pipeline;  // NOLINT(build/namespaces)

class CpuQoSPipelineConfig : public SerenityConfig {
 public:
  CpuQoSPipelineConfig() {}

  explicit CpuQoSPipelineConfig(const SerenityConfig& customCfg) {
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
 *       /           \______________________
 *       |           |                      \
 *       | {{ Too Low Usage Filter }}       |
 *       |           |                      |
 *       |    |ResourceUsage|               |
 *       |           |                      |
 *       |    {{ IPC EMA Filter }}          |
 *       |           |          \           |
 * |ResourceUsage|   |      |ResourceUsage| - {{EMA Resource Usage Export}}
 *       |           |                      |
 *       |           |            {{ Cpu Usage EMA Filter }}
 *       |           |                      |
 *       |     |ResourceUsage|        |ResourceUsage|
 *       |           |                      |
 *       |           |        {{ Too High Utilization Detector }}
 *       |           |                      |
 *       |  {{ IPC Signal Detector<Drop> }}  |
 *       |           |                      |
 *       |      |Contentions|          |Contentions|
 *       |           |                      |
 *       \___________|____________________  |
 *             \     |                    \ |
 *  {{ IPC QoS Observer }}      {{ CPU QoS Observer }}
 *              \______________________/
 *                     |Corrections|
 *                          |
 *                  {{ PIPELINE SINK }}
 *
 * For detailed schema please see: docs/pipeline.md
 */
class CpuQoSPipeline : public QoSControllerPipeline {
 public:
  explicit CpuQoSPipeline(const SerenityConfig& _conf)
    : conf(CpuQoSPipelineConfig(_conf)),
      // Time series exporters.
      rawResourcesExporter("raw"),
      emaFilteredResourcesExporter("ema"),
      // NOTE(bplotka): age Filter should initialized first before passing
      // to the qosCorrectionObserver.
      ageFilter(),
      // Last item in pipeline.
      correctionMerger(
          this, 2,  // Two producers connected. IPC & CPU observers.
          Tag(QOS_CONTROLLER, "CorrectionMerger")),
      ipcCorrectionObserver(
          &correctionMerger, 1,
          &ageFilter,
          // TODO(Bplotka): Change to own Ipc strategy.
          new SeniorityStrategy(conf[SeniorityStrategy::NAME])),
      ipcDropDetector(
          &ipcCorrectionObserver,
          usage::getEmaIpc,
          conf[SIGNAL_DROP_ANALYZER_NAME],
          Tag(QOS_CONTROLLER, "IPC detectorFilter"),
          Contention_Type_IPC),
      ipcEMAFilter(
          &ipcDropDetector,
          usage::getIpc,
          usage::setEmaIpc,
          conf.getD(ema::ALPHA_IPC),
          Tag(QOS_CONTROLLER, "ipcEMAFilter")),
      tooLowUsageFilter(
          &ipcEMAFilter,
          conf[TooLowUsageFilter::NAME],
          Tag(QOS_CONTROLLER, "tooLowCPUUsageFilter")),
      cpuCorrectionObserver(
          &correctionMerger, 1,
          &ageFilter,
        new CpuContentionStrategy(
            conf[CpuContentionStrategy::NAME],
            usage::getEmaCpuUsage)),
      cpuUtilizationDetector(
          &cpuCorrectionObserver,
          usage::getEmaCpuUsage,
          conf[TooHighCpuUsageDetector::NAME],
          Tag(QOS_CONTROLLER, "CPU High Usage utilization detector")),
      cpuEMAFilter(
          &cpuUtilizationDetector,
          usage::getCpuUsage,
          usage::setEmaCpuUsage,
          conf.getD(ema::ALPHA_CPU),
          Tag(QOS_CONTROLLER, "cpuEMAFilter")),
      cumulativeFilter(
          &tooLowUsageFilter,
          Tag(QOS_CONTROLLER, "cumulativeFilter")),
      // First item in pipeline. For now, close the pipeline for QoS.
      valveFilter(
          &cumulativeFilter,
          conf.getB(VALVE_OPENED),
          Tag(QOS_CONTROLLER, "valveFilter")) {
    this->ageFilter.addConsumer(&valveFilter);
    // Setup starting producer.
    this->addConsumer(&ageFilter);

    // QoSCorrection observers needs ResourceUsage as well.
    cumulativeFilter.addConsumer(&cpuCorrectionObserver);
    cumulativeFilter.addConsumer(&ipcCorrectionObserver);
    cumulativeFilter.addConsumer(&cpuEMAFilter);

    // Setup Time Series export
    if (conf.getB(ENABLED_VISUALISATION)) {
      this->addConsumer(&rawResourcesExporter);
      ipcEMAFilter.addConsumer(&emaFilteredResourcesExporter);
    }
  }

  virtual Try<Nothing> postPipelineRun() {
    this->cpuCorrectionObserver.reset();
    this->ipcCorrectionObserver.reset();
    // Force pipeline continuation.
    // TODO(bplotka): That would not be needed if we always continue pipeline.
    return this->correctionMerger.ensure();
  }

 private:
  SerenityConfig conf;
  // --- Filters ---
  ExecutorAgeFilter ageFilter;
  CumulativeFilter cumulativeFilter;
  EMAFilter cpuEMAFilter;
  EMAFilter ipcEMAFilter;
  SignalBasedDetector ipcDropDetector;
  TooHighCpuUsageDetector cpuUtilizationDetector;
  TooLowUsageFilter tooLowUsageFilter;
  ValveFilter valveFilter;
  CorrectionMergerFilter correctionMerger;

  // --- Observers ---
  QoSCorrectionObserver ipcCorrectionObserver;
  QoSCorrectionObserver cpuCorrectionObserver;

  // --- Time Series Exporters ---
  ResourceUsageTimeSeriesExporter rawResourcesExporter;
  ResourceUsageTimeSeriesExporter emaFilteredResourcesExporter;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_CPU_QOS_PIPELINE_HPP
