#ifndef SERENITY_QOS_PIPELINE_HPP
#define SERENITY_QOS_PIPELINE_HPP

#include "contention_detectors/signal_based.hpp"
#include "contention_detectors/overload.hpp"
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

#include "observers/strategies/cache_occupancy.hpp"
#include "observers/strategies/cpu_contention.hpp"
#include "observers/strategies/seniority.hpp"

#include "serenity/config.hpp"
#include "serenity/data_utils.hpp"
#include "serenity/serenity.hpp"

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
    this->items[ema::ALPHA] = ema::DEFAULT_ALPHA;
    this->items[VALVE_OPENED] = DEFAULT_VALVE_OPENED;
    this->items[ENABLED_VISUALISATION] = DEFAULT_ENABLED_VISUALISATION;
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
    : conf(_conf),
      // NOTE(bplotka): age Filter should initialized first before passing
      // to the qosCorrectionObserver.
      ageFilter(),
      // Last item in pipeline.
      correctionMerger(
          this,
          Tag(QOS_CONTROLLER, "CorrectionMerger")),
      // ipcContentionObserver(
      //     &correctionMerger,
      //     &ageFilter,
      //     new SeniorityStrategy(conf[SeniorityStrategy::NAME]),
      //     strategy::DEFAULT_CONTENTION_COOLDOWN,
      //     Tag(QOS_CONTROLLER, SeniorityStrategy::NAME)),
      cacheOccupancyContentionObserver(
          &correctionMerger,
          &ageFilter,
          new CacheOccupancyStrategy(),
          strategy::DEFAULT_CONTENTION_COOLDOWN,
          Tag(QOS_CONTROLLER, CacheOccupancyStrategy::NAME)),
      ipcDropDetector(
          &cacheOccupancyContentionObserver,
          usage::getEmaIpc,
          conf.getSectionOrNew(SIGNAL_DROP_ANALYZER_NAME),
          Tag(QOS_CONTROLLER, "IPC detectorFilter"),
          Contention_Type_IPC),
      ipcEMAFilter(
          &ipcDropDetector,
          usage::getIpc,
          usage::setEmaIpc,
          conf.getItemOrDefault<double_t>(ema::ALPHA_IPC,
                                          ema::DEFAULT_ALPHA_IPC),
          Tag(QOS_CONTROLLER, "ipcEMAFilter")),
      tooLowUsageFilter(
          &ipcEMAFilter,
          conf.getSectionOrNew(TooLowUsageFilter::NAME),
          Tag(QOS_CONTROLLER, "tooLowCPUUsageFilter")),
      cpuContentionObserver(
          &correctionMerger,
          &ageFilter,
          new CpuContentionStrategy(
            conf.getSectionOrNew(CpuContentionStrategy::NAME),
            usage::getEmaCpuUsage),
          strategy::DEFAULT_CONTENTION_COOLDOWN,
          Tag(QOS_CONTROLLER, CpuContentionStrategy::NAME)),
      overloadDetector(
          &cpuContentionObserver,
          usage::getEmaCpuUsage,
          conf.getSectionOrNew(OverloadDetector::NAME),
          Tag(QOS_CONTROLLER, "CPU High Usage utilization detector")),
      cpuEMAFilter(
          &overloadDetector,
          usage::getCpuUsage,
          usage::setEmaCpuUsage,
          conf.getItemOrDefault<double_t>(ema::ALPHA_CPU,
                                          ema::DEFAULT_ALPHA_CPU),
          Tag(QOS_CONTROLLER, "cpuEMAFilter")),
      cumulativeFilter(
          &tooLowUsageFilter,
          Tag(QOS_CONTROLLER, "cumulativeFilter")),
      // First item in pipeline. For now, close the pipeline for QoS.
      valveFilter(
          &cumulativeFilter,
          conf.getItemOrDefault<bool>(VALVE_OPENED, DEFAULT_VALVE_OPENED),
          Tag(QOS_CONTROLLER, "valveFilter")) {
    this->ageFilter.addConsumer(&valveFilter);
    // Setup starting producer.
    this->addConsumer(&ageFilter);

    // cacheOccupancyContentionObserver.
    // Producer<Contentions>::addConsumer(&ipcContentionObserver);

    // QoSCorrection observers needs ResourceUsage as well.
    cpuEMAFilter.addConsumer(&cpuContentionObserver);
    // cumulativeFilter.addConsumer(&ipcContentionObserver);
    cumulativeFilter.addConsumer(&cacheOccupancyContentionObserver);
    cumulativeFilter.addConsumer(&cpuEMAFilter);
  }

 private:
  SerenityConfig conf;

  // --- Shared resource contention QoS
  CorrectionMergerFilter correctionMerger;
  // QoSCorrectionObserver ipcContentionObserver;
  QoSCorrectionObserver cacheOccupancyContentionObserver;

  SignalBasedDetector ipcDropDetector;
  EMAFilter ipcEMAFilter;
  TooLowUsageFilter tooLowUsageFilter;

  // --- Node overload QoS
  QoSCorrectionObserver cpuContentionObserver;
  OverloadDetector overloadDetector;
  EMAFilter cpuEMAFilter;

  CumulativeFilter cumulativeFilter;
  ExecutorAgeFilter ageFilter;

  ValveFilter valveFilter;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_QOS_PIPELINE_HPP
