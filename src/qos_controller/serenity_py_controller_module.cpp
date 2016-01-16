#include <string>
#include <memory>

#include "filters/pypeline.hpp"

#include "mesos/mesos.hpp"
#include "mesos/module.hpp"

#include "mesos/module/qos_controller.hpp"
#include "mesos/slave/qos_controller.hpp"

#include "pipeline/python_qos_pipeline.hpp"

#include "serenity/config.hpp"

#include "stout/try.hpp"

#include "qos_controller/serenity_controller.hpp"

// TODO(nnielsen): Should be explicit using-directives.
using namespace mesos;  // NOLINT(build/namespaces)
using namespace mesos::serenity::python_pipeline;  // NOLINT(build/namespaces)

using mesos::serenity::PythonQoSPipeline;
using mesos::serenity::SerenityConfig;
using mesos::serenity::SerenityController;
using mesos::serenity::QoSControllerPipeline;
using mesos::serenity::PypelineFilter;

using mesos::slave::QoSController;

static QoSController* createSerenityPyController(
    const Parameters& parameters) {
  LOG(INFO) << "Loading Serenity QoS Py Controller module";
  // TODO(bplotka): Fetch configuration from parameters or conf file.
  //
  // --Hardcoded configuration for Serenity QoS Py Controller---

  SerenityConfig conf;
  conf[PypelineFilter::NAME].set(SERENITY_PYPELINE_PATH,
                           DEFAULT_SERENITY_PYPELINE_PATH);

  // Since slave is configured for 5 second perf interval, it is useless to
  // check correction more often then 5 sec.
  double onEmptyCorrectionInterval = 2;

  // --End of hardcoded configuration for Serenity QoS Controller---

  // Use static constructor of QoSController.
  Try<QoSController*> result =
    SerenityController::create(
      std::shared_ptr<QoSControllerPipeline>(
          new PythonQoSPipeline(conf)),
          onEmptyCorrectionInterval);

  if (result.isError()) {
    return NULL;
  }
  return result.get();
}

mesos::modules::Module<QoSController> com_gut_mesos_SerenityPyController(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "bplotka",
    "bwplotka@<...>",
    "Serenity QoS Py Controller",
    NULL,
    createSerenityPyController);
