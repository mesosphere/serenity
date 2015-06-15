#include <string>

#include <mesos/mesos.hpp>
#include <mesos/module.hpp>

#include <mesos/module/resource_estimator.hpp>

#include <mesos/slave/resource_estimator.hpp>

#include <stout/try.hpp>
#include <mesos/slave/qos_controller.hpp>

#include "estimator/serenity_estimator.hpp"

using namespace mesos;

using mesos::serenity::SerenityController;

using mesos::slave::QoSController;

static QoSController* createSerenityController(const Parameters& parameters)
{
  LOG(INFO) << "Loading Serenity Estimator module";
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
