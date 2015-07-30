#include <string>
#include <memory>

#include "filters/drop.hpp"

#include "mesos/mesos.hpp"
#include "mesos/module.hpp"

#include "mesos/module/qos_controller.hpp"
#include "mesos/slave/qos_controller.hpp"

#include "pipeline/qos_pipeline.hpp"

#include "stout/try.hpp"

#include "qos_controller/serenity_controller.hpp"

// TODO(nnielsen): Should be explicit using-directives.
using namespace mesos;  // NOLINT(build/namespaces)

using mesos::serenity::ChangePointDetectionState;
using mesos::serenity::CpuQoSPipeline;
using mesos::serenity::RollingChangePointDetector;
using mesos::serenity::SerenityController;
using mesos::serenity::QoSControllerPipeline;

using mesos::slave::QoSController;


static std::shared_ptr<QoSControllerPipeline>
  configureControllerPipelineFromParams(
    const Parameters& parameters) {
  // TODO(bplotka): Fetch configuration parameters to customize IpcDrop
  // TODO(bplotka): Obtain the type of pipeline from parameters.
  std::shared_ptr<QoSControllerPipeline> pipeline(
      new CpuQoSPipeline<RollingChangePointDetector>(
          ChangePointDetectionState::createForRollingDetector(10, 10, 0.5)));
  return pipeline;
}

static QoSController* createSerenityController(const Parameters& parameters) {
  LOG(INFO) << "Loading Serenity QoS Controller module";

  Try<QoSController*> result = SerenityController::create(
      configureControllerPipelineFromParams(parameters));
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
