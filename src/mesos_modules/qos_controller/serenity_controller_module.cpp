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
using namespace mesos::serenity::ema;  // NOLINT(build/namespaces)
using namespace mesos::serenity::strategy;  // NOLINT(build/namespaces)
using namespace mesos::serenity::detector;  // NOLINT(build/namespaces)
using namespace mesos::serenity::too_low_usage;  // NOLINT(build/namespaces)
using namespace mesos::serenity::qos_pipeline;  // NOLINT(build/namespaces)

using mesos::serenity::CpuContentionStrategy;
using mesos::serenity::CpuQoSPipeline;
using mesos::serenity::SerenityConfig;
using mesos::serenity::SerenityController;
using mesos::serenity::SeniorityStrategy;
using mesos::serenity::SignalBasedDetector;
using mesos::serenity::TooLowUsageFilter;
using mesos::serenity::QoSControllerPipeline;

using mesos::slave::QoSController;


// IPC QoS pipeline.
static QoSController* createSerenityController(
    const Parameters& parameters) {
  LOG(INFO) << "Loading Serenity QoS Controller module";
  // TODO(bplotka): Fetch configuration from parameters or conf file.

  SerenityConfig conf;
  double onEmptyCorrectionInterval =
    conf.getItemOrDefault<double_t>(ON_EMPTY_CORRECTION_INTERVAL,
                        DEFAULT_ON_EMPTY_CORRECTION_INTERVAL);

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
