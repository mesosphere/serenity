#include <string>
#include <memory>

#include "filters/drop.hpp"

#include "mesos/mesos.hpp"
#include "mesos/module.hpp"

#include "mesos/module/qos_controller.hpp"
#include "mesos/slave/qos_controller.hpp"

#include "pipeline/qos_pipeline.hpp"

#include "serenity/config.hpp"

#include "stout/try.hpp"

#include "qos_controller/serenity_controller.hpp"

// TODO(nnielsen): Should be explicit using-directives.
using namespace mesos;  // NOLINT(build/namespaces)

using mesos::serenity::ChangePointDetectionState;
using mesos::serenity::IpsQoSPipeline;
using mesos::serenity::RollingFractionalChangePointDetector;
using mesos::serenity::SerenityController;
using mesos::serenity::QoSControllerPipeline;
using mesos::serenity::QoSPipelineConf;

using mesos::slave::QoSController;

static QoSController* createSerenityController(const Parameters& parameters) {
  LOG(INFO) << "Loading Serenity QoS Controller module";
  // TODO(bplotka): Fetch configuration from parameters or conf file
  // to customize IpsDrop

  // --Hardcoded configuration for Serenity QoS Controller---

  QoSPipelineConf conf;
  ChangePointDetectionState cpdState;
  // Detector configuration:
  // How far we look back in samples.
  cpdState.windowSize = 10;
  // How many iterations detector will wait with creating another
  // contention.
  cpdState.contentionCooldown = 10;
  // Defines how much (relatively to base point) value must drop to trigger
  // contention.
  // Most detectors will use that.
  cpdState.fractionalThreshold = 0.5;

  conf.cpdState = cpdState;
  conf.emaAlpha = 0.2;
  conf.visualisation = true;
  // Let's start with QoS pipeline disabled.
  conf.valveOpened = false;

  // Since slave is configured for 5 second perf interval, it is useless to
  // check correction more often then 5 sec.
  double onEmptyCorrectionInterval = 5;

  // --End of hardcoded configuration for Serenity QoS Controller---

  Try<QoSController*> result =
    SerenityController::create(
        std::shared_ptr<QoSControllerPipeline>(
            new IpsQoSPipeline<RollingFractionalChangePointDetector>(conf)),
        onEmptyCorrectionInterval);

  if (result.isError()) {
    return NULL;
  }
  return result.get();
}


mesos::modules::Module<QoSController> com_mesosphere_mesos_SerenityController(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Mesosphere & Intel",
    "support@mesosphere.com",
    "Serenity QoS Controller",
    NULL,
    createSerenityController);
