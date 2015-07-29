#ifndef SERENITY_QOS_PIPELINE_HPP
#define SERENITY_QOS_PIPELINE_HPP

#include "filters/drop.hpp"
#include "filters/utilization_threshold.hpp"
#include "filters/valve.hpp"

#include "messages/serenity.hpp"

#include "pipeline/pipeline.hpp"

#include "observers/qos_correction.hpp"

#include "serenity/data_utils.hpp"
#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

using QoSControllerPipeline = Pipeline<ResourceUsage, QoSCorrections>;

/**
 * Pipeline which includes necessary filters for making  QoS Corrections
 * based on CPU contentions.
 * It includes:
 * - utilizationFilter
 * - valveFilter
 * - ipcDropFilter
 * - qosCorrectionObserver
 */
template<class Detector>
class CpuQoSPipeline : public QoSControllerPipeline {
  static_assert(std::is_base_of<ChangePointDetector, Detector>::value,
                "Detector must derive from ChangePointDetector");

 public:
  explicit CpuQoSPipeline(ChangePointDetectionState _cpdState) :
      cpdState(_cpdState),
      // Last item in pipeline.
      qoSCorrectionObserver(this, 1),
      // Item before QoSCorrection observer.
      ipcDropFilter(&qoSCorrectionObserver, usage::getIpc, cpdState),
      valveFilter(ValveFilter(
          &ipcDropFilter, ValveType::RESOURCE_ESTIMATOR_VALVE)),
      // First item in pipeline.
      utilizationFilter(&valveFilter, DEFAULT_UTILIZATION_THRESHOLD) {
    // Setup starting producer.
    this->addConsumer(&utilizationFilter);

    // QoSCorrection needs ResourceUsage as well.
    valveFilter.addConsumer(&qoSCorrectionObserver);
  }

 private:
  ChangePointDetectionState cpdState;
  // --- Filters ---
  DropFilter<Detector> ipcDropFilter;
  UtilizationThresholdFilter utilizationFilter;
  ValveFilter valveFilter;

  // --- Observers ---
  QoSCorrectionObserver qoSCorrectionObserver;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_QOS_PIPELINE_HPP
