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
using mesos::serenity::CpuQoSPipeline;
using mesos::serenity::IpsQoSPipeline;
using mesos::serenity::RollingFractionalDetector;
using mesos::serenity::SerenityController;
using mesos::serenity::QoSControllerPipeline;
using mesos::serenity::QoSPipelineConf;

using mesos::slave::QoSController;

// IPC pipeline.
static QoSController* createIpcSerenityController(
    const Parameters& parameters) {
  LOG(INFO) << "Loading Serenity QoS Controller module";
  // TODO(bplotka): Fetch configuration from parameters or conf file
  // to customize IpcDrop

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
  cpdState.fractionalThreshold = 0.3;
  // Defines how to convert difference in values to CPU.
  // This option helps RollingFractionalDetector to estimate severity of
  // drop.
  cpdState.severityLevel = 0.4;  // 0.4 IPC drop means ~ 1 CPU to kill.

  conf.cpdState = cpdState;
  conf.emaAlpha = 0.9;
  conf.visualisation = true;
  conf.valveOpened = true;

  // Since slave is configured for 5 second perf interval, it is useless to
  // check correction more often then 5 sec.
  double onEmptyCorrectionInterval = 5;

  // --End of hardcoded configuration for Serenity QoS Controller---

  Try<QoSController*> result =
      SerenityController::create(
          std::shared_ptr<QoSControllerPipeline>(
              new CpuQoSPipeline<RollingFractionalDetector>(conf)),
          onEmptyCorrectionInterval);

  if (result.isError()) {
    return NULL;
  }
  return result.get();
}


// IPS pipeline
static QoSController* createIpsSerenityController(
    const Parameters& parameters) {
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
  // Defines how many instructions can be done per one CPU in one second.
  // This option helps RollingFractionalDetector to estimate severity of
  // drop.
  cpdState.severityLevel = 1000000000;  // 1 Billion.

  conf.cpdState = cpdState;
  conf.emaAlpha = 0.8;
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
            new IpsQoSPipeline<RollingFractionalDetector>(conf)),
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
    createIpcSerenityController);
