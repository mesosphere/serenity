#ifndef SERENITY_VALVE_FILTER_HPP
#define SERENITY_VALVE_FILTER_HPP

#include <string>

#include "process/future.hpp"
#include "process/owned.hpp"

#include "stout/lambda.hpp"

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

enum ValveType {
  RESOURCE_ESTIMATOR_VALVE,
  QOS_CONTROLLER_VALVE
};


const std::string PIPELINE_ENABLE_KEY = "enabled";
const std::string VALVE_ROUTE = "/valve";
const std::string RESOURCE_ESTIMATOR_PROCESS_BASE =
    "serenity_resource_estimator";
const std::string QOS_CONTROLLER_PROCESS_BASE =
    "serenity_qos_controller";


static const std::string getProcessBaseName(ValveType valveType) {
  switch (valveType) {
    case RESOURCE_ESTIMATOR_VALVE:
      return RESOURCE_ESTIMATOR_PROCESS_BASE;
    case QOS_CONTROLLER_VALVE:
      return QOS_CONTROLLER_PROCESS_BASE;
  }
}


// Forward declaration.
class ValveFilterEndpointProcess;


class ValveFilter :
    public Consumer<ResourceUsage>, public Producer<ResourceUsage> {
 public:
  ValveFilter(
      Consumer<ResourceUsage>* _consumer,
      ValveType valveType,
      bool _opened = true);

  ~ValveFilter();

  Try<Nothing> consume(const ResourceUsage& in);

 private:
  lambda::function<process::Future<bool>()> isOpened;
  process::Owned<ValveFilterEndpointProcess> process;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_VALVE_FILTER_HPP
