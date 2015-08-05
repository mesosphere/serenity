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


template<class Type>
static const std::string getValveProcessBaseName() {
  switch (Type::type) {
    case RESOURCE_ESTIMATOR:
      return RESOURCE_ESTIMATOR_VALVE_PROCESS_BASE;
    case QOS_CONTROLLER:
      return QOS_CONTROLLER_VALVE_PROCESS_BASE;
  }
}


// Forward declaration
template<class Type>
class ValveFilterEndpointProcess;

template<class Type>
class ValveFilter :
    public Consumer<ResourceUsage>, public Producer<ResourceUsage> {
 public:
  explicit ValveFilter(bool _opened = true);

  ValveFilter(
      Consumer<ResourceUsage>* _consumer,
      bool _opened = true);

  ~ValveFilter();

  Try<Nothing> consume(const ResourceUsage& in);

  static constexpr const char* const name() {
    return (Type::name + std::string("ValveFilter: ")).c_str();
  }

 private:
  lambda::function<process::Future<bool>()> isOpened;
  process::Owned<ValveFilterEndpointProcess<Type>> process;
};

#define VALVE_FILTER_NAME ValveFilter<Type>::name()

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_VALVE_FILTER_HPP
