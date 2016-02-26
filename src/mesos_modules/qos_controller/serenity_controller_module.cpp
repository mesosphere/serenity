#include <string>
#include <memory>

#include "mesos/mesos.hpp"
#include "mesos/module.hpp"

#include "mesos/module/qos_controller.hpp"
#include "mesos/slave/qos_controller.hpp"

#include "mesos_modules/qos_controller/serenity_controller.hpp"

#include "pipeline/qos_pipeline.hpp"

#include "serenity/config.hpp"

#include "stout/try.hpp"

// TODO(nnielsen): Should be explicit using-directives.
using namespace mesos;  // NOLINT(build/namespaces)

using mesos::serenity::CpuContentionStrategy;
using mesos::serenity::CpuQoSPipeline;
using mesos::serenity::Config;
using mesos::serenity::ConfigValidator;
using mesos::serenity::SerenityController;
using mesos::serenity::SeniorityStrategy;
using mesos::serenity::SignalBasedDetector;
using mesos::serenity::TooLowUsageFilter;
using mesos::serenity::QoSControllerPipeline;

using mesos::slave::QoSController;

const constexpr char* ON_EMPTY_CORRECTION_INTERVAL_KEY =
  "ON_EMPTY_CORRECTION_INTERVAL";

const constexpr double_t ON_EMPTY_CORRECTION_INTERVAL_DEFAULT = 2;

// IPC QoS pipeline.
static QoSController* createSerenityController(
    const Parameters& parameters) {
  LOG(INFO) << "Loading Serenity QoS Controller module";
  // TODO(bplotka): Fetch configuration from parameters or conf file.
  Config conf;

  double_t onEmptyCorrectionInterval =
    ConfigValidator<double_t>(
        conf.getValue<double_t>(ON_EMPTY_CORRECTION_INTERVAL_KEY))
          .getOrElse(ON_EMPTY_CORRECTION_INTERVAL_DEFAULT);

  // Use static constructor of QoSController.
  Try<QoSController*> result =
    SerenityController::create(std::unique_ptr<QoSControllerPipeline>(
                                 new CpuQoSPipeline(conf)),
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
