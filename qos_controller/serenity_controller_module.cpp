#include <mesos/mesos.hpp>
#include <mesos/module.hpp>

#include <mesos/module/qos_controller.hpp>

#include <stout/try.hpp>
#include <mesos/slave/qos_controller.hpp>

#include <string>

#include "qos_controller/serenity_controller.hpp"

// TODO(nnielsen): Should be explicit using-directives.
using namespace mesos;  // NOLINT(build/namespaces)

using mesos::serenity::SerenityController;

using mesos::slave::QoSController;

static QoSController* createSerenityController(const Parameters& parameters) {
  LOG(INFO) << "Loading Serenity QoS Controller module";
  Try<QoSController*> result = SerenityController::create(None());
  if (result.isError()) {
    return NULL;
  }
  return result.get();
}


mesos::modules::Module<QoSController> com_mesosphere_mesos_SerenityController(
    MESOS_MODULE_API_VERSION,
    MESOS_VERSION,
    "Mesosphere",
    "support@mesosphere.com",
    "Serenity QoS Controller",
    NULL,
    createSerenityController);
