#ifndef SERENITY_VALVE_FILTER_HPP
#define SERENITY_VALVE_FILTER_HPP

#include <string>

#include "mesos/mesos.hpp"

#include "process/future.hpp"
#include "process/owned.hpp"

#include "serenity/serenity.hpp"

#include "stout/lambda.hpp"

namespace mesos {
namespace serenity {

const std::string PIPELINE_ENABLE_KEY = "enabled";
const std::string VALVE_ROUTE = "/valve";
const std::string RESOURCE_ESTIMATOR_VALVE_PROCESS_BASE =
    "serenity_resource_estimator";
const std::string QOS_CONTROLLER_VALVE_PROCESS_BASE =
    "serenity_qos_controller";


static const std::string getValveProcessBaseName(const ModuleType type) {
  switch (type) {
    case RESOURCE_ESTIMATOR:
      return RESOURCE_ESTIMATOR_VALVE_PROCESS_BASE;
    case QOS_CONTROLLER:
      return QOS_CONTROLLER_VALVE_PROCESS_BASE;
    default:
      return "serenity_valve";
  }
}


// Forward declaration
class ValveFilterEndpointProcess;

class ValveFilter :
    public Consumer<ResourceUsage>, public Producer<ResourceUsage> {
 public:
  explicit ValveFilter(bool _opened = true,
                       const Tag& _tag = Tag(UNDEFINED, "valveFilter"));

  ValveFilter(
      Consumer<ResourceUsage>* _consumer,
      bool _opened = true,
      const Tag& _tag = Tag(UNDEFINED, "valveFilter"));

  ~ValveFilter();

  Try<Nothing> consume(const ResourceUsage& in);

 private:
  const Tag tag;
  lambda::function<process::Future<bool>()> isOpened;
  process::Owned<ValveFilterEndpointProcess> process;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_VALVE_FILTER_HPP
