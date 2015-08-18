#ifndef SERENITY_QOS_PIPELINE_HPP
#define SERENITY_QOS_PIPELINE_HPP

#include "filters/ema.hpp"
#include "filters/drop.hpp"
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

using QoSControllerPipeline = Pipeline<ResourceUsage, QoSCorrections>;

/**
 * Pipeline which includes necessary filters for making  QoS Corrections
 * based on CPU contentions.
 *   {{ PIPELINE SOURCE }}
 *            |           \
 *      |ResourceUsage|  |ResourceUsage| - {{ Raw Resource Usage Export }}
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
template<class Detector>
class CpuQoSPipeline : public QoSControllerPipeline {
  static_assert(std::is_base_of<ChangePointDetector, Detector>::value,
                "Detector must derive from ChangePointDetector");

 public:
  explicit CpuQoSPipeline(QoSPipelineConf _conf)
    : conf(_conf),
      // Time series exporters.
      rawResourcesExporter("raw"),
      emaFilteredResourcesExporter("ema"),
      // Last item in pipeline.
      qoSCorrectionObserver(this, 1),
      ipcDropFilter(
          &qoSCorrectionObserver,
          usage::getEmaIpc,
          conf.cpdState,
          Tag(QOS_CONTROLLER, "IPC dropFilter")),
      emaFilter(
          &ipcDropFilter,
          usage::getIpc,
          usage::setEmaIpc,
          conf.emaAlpha,
          Tag(QOS_CONTROLLER, "emaFilter")),
      prExecutorPassFilter(&emaFilter) {
      // First item in pipeline. For now, close the pipeline for QoS.
//      valveFilter(
//          &prExecutorPassFilter,
//          conf.valveOpened,
//          Tag(QOS_CONTROLLER, "valveFilter")) {
    // Setup starting producer.
    this->addConsumer(&prExecutorPassFilter);

    // QoSCorrection needs ResourceUsage as well.
    this->addConsumer(&qoSCorrectionObserver);

    // Setup Time Series export
    if (conf.visualisation) {
      this->addConsumer(&rawResourcesExporter);
      emaFilter.addConsumer(&emaFilteredResourcesExporter);
    }
  }

  virtual Try<Nothing> resetSyncConsumers() {
    this->qoSCorrectionObserver.reset();

    return Nothing();
  }

 private:
  QoSPipelineConf conf;
  // --- Filters ---
  EMAFilter emaFilter;
  DropFilter<Detector> ipcDropFilter;
  PrExecutorPassFilter prExecutorPassFilter;
//  ValveFilter valveFilter;

  // --- Observers ---
  QoSCorrectionObserver qoSCorrectionObserver;

  // --- Time Series Exporters ---
  ResourceUsageTimeSeriesExporter rawResourcesExporter;
  ResourceUsageTimeSeriesExporter emaFilteredResourcesExporter;
};


/**
 * Pipeline which includes necessary filters for making  QoS Corrections
 * based on CPU contentions. (IPS signal)
 *   {{ PIPELINE SOURCE }}
 *            |           \
 *      |ResourceUsage|  |ResourceUsage| - {{ Raw Resource Usage Export }}
 *            |
 *       {{ Valve }} (+http endpoint) // First item.
 *            |
 *      |ResourceUsage|
 *       /          \
 *       |  {{ OnlyPRTaskFilter }}
 *       |           |
 *       |     |ResourceUsage|
 *       |           |
 *       |    {{ IPS EMA Filter }}
 *       |           |          \
 *       |     |ResourceUsage|  |ResourceUsage| - {{EMA Resource Usage Export}}
 *       |           |
 *       |     {{ IPS Drop<ChangePointDetector> }}
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
// template<class Detector>
// class IpsQoSPipeline : public QoSControllerPipeline {
//  static_assert(std::is_base_of<ChangePointDetector, Detector>::value,
//                "Detector must derive from ChangePointDetector");
//
// public:
//  explicit IpsQoSPipeline(QoSPipelineConf _conf)
//      : conf(_conf),
//      // Time series exporters.
//        rawResourcesExporter("raw"),
//        emaFilteredResourcesExporter("ema"),
//      // Last item in pipeline.
//        qoSCorrectionObserver(this, 1),
//        ipsDropFilter(
//            &qoSCorrectionObserver,
//            usage::getEmaIps,
//            conf.cpdState,
//            Tag(QOS_CONTROLLER, "IPS dropFilter")),
//        emaFilter(
//            &ipsDropFilter,
//            usage::getIps,
//            usage::setEmaIps,
//            conf.emaAlpha,
//            Tag(QOS_CONTROLLER, "emaFilter")),
//        prExecutorPassFilter(&emaFilter),
//      // First item in pipeline. For now, close the pipeline for QoS.
//        valveFilter(
//            &prExecutorPassFilter,
//            conf.valveOpened,
//            Tag(QOS_CONTROLLER, "valveFilter")) {
//    // Setup starting producer.
//    this->addConsumer(&valveFilter);
//
//    // QoSCorrection needs ResourceUsage as well.
//    valveFilter.addConsumer(&qoSCorrectionObserver);
//
//    // Setup Time Series export
//    if (conf.visualisation) {
//      this->addConsumer(&rawResourcesExporter);
//      emaFilter.addConsumer(&emaFilteredResourcesExporter);
//    }
//  }
//
//  virtual Try<Nothing> resetSyncConsumers() {
//    this->qoSCorrectionObserver.reset();
//
//    return Nothing();
//  }
//
// private:
//  QoSPipelineConf conf;
//  // --- Filters ---
//  EMAFilter emaFilter;
//  DropFilter<Detector> ipsDropFilter;
//  PrExecutorPassFilter prExecutorPassFilter;
//  ValveFilter valveFilter;
//
//  // --- Observers ---
//  QoSCorrectionObserver qoSCorrectionObserver;
//
//  // --- Time Series Exporters ---
//  ResourceUsageTimeSeriesExporter rawResourcesExporter;
//  ResourceUsageTimeSeriesExporter emaFilteredResourcesExporter;
//};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_QOS_PIPELINE_HPP
