#include "serenity_qos_controller.hpp"

namespace mesos {
namespace serenity {

SerenityQoSController::~SerenityQoSController() {}

Try<Nothing> SerenityQoSController::consume(int in) {
  return Nothing();
}

} // namespace serenity
} // namespace mesos

using namespace mesos;

//using mesos::slave::Isolator;

//static SerenityController *createQosController(const Parameters &parameters) {
//  LOG(INFO) << "Loading Serenity QoS Controller module";
//  Try < QoSController * > result = SerenityEstimator::create(parameters);
//  if (result.isError()) {
//    return NULL;
//  }
//  return result.get();
//}
//
//
//mesos::modules::Module <QoSController> com_mesosphere_mesos_SerenityQoSController(
//    MESOS_MODULE_API_VERSION,
//    MESOS_VERSION,
//    "Mesosphere",
//    "support@mesosphere.com",
//    "Serenity Estimator",
//    NULL,
//    createEstimator);
//
