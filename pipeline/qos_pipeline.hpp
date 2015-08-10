#ifndef SERENITY_QOS_PIPELINE_HPP
#define SERENITY_QOS_PIPELINE_HPP

#include "filters/ema.hpp"
#include "filters/drop.hpp"
#include "filters/utilization_threshold.hpp"
#include "filters/valve.hpp"

#include "messages/serenity.hpp"

#include "pipeline/pipeline.hpp"

#include "observers/qos_correction.hpp"

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
 *            |
 * {{ Utilization Observer }}
 *            |
 *      |ResourceUsage|
 *       /          \
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
  explicit CpuQoSPipeline(
      ChangePointDetectionState _cpdState,
      bool _visualisation = true,
      bool _valveOpened = false)
    : cpdState(_cpdState),
      // Time series exporters.
      rawResourcesExporter("raw"),
      emaFilteredResourcesExporter("ema"),
      // Last item in pipeline.
      qoSCorrectionObserver(this, 1),
      ipcDropFilter(&qoSCorrectionObserver,
                    usage::getEmaIpc,
                    cpdState),
      emaFilter(&ipcDropFilter,
                usage::getIpc,
                usage::setEmaIpc,
                DEFAULT_EMA_FILTER_ALPHA,
                Tag(QOS_CONTROLLER, "emaFilter")),
      utilizationFilter(&emaFilter,
                        DEFAULT_UTILIZATION_THRESHOLD,
                        Tag(QOS_CONTROLLER, "utiliationFilter")),
      // First item in pipeline. For now, close the pipeline for QoS.
      valveFilter(&utilizationFilter,
                  _valveOpened,
                  Tag(QOS_CONTROLLER, "valveFilter")) {
    // Setup starting producer.
    this->addConsumer(&valveFilter);

    // QoSCorrection needs ResourceUsage as well.
    valveFilter.addConsumer(&qoSCorrectionObserver);

    // Setup Time Series export
    if (_visualisation) {
      this->addConsumer(&rawResourcesExporter);
      emaFilter.addConsumer(&emaFilteredResourcesExporter);
    }
  }

  virtual Try<Nothing> resetSyncConsumers() {
    this->qoSCorrectionObserver.reset();

    return Nothing();
  }

 private:
  ChangePointDetectionState cpdState;
  // --- Filters ---
  EMAFilter emaFilter;
  DropFilter<Detector> ipcDropFilter;
  UtilizationThresholdFilter utilizationFilter;
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
