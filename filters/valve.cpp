#include <atomic>
#include <string>

#include "glog/logging.h"

#include "mesos/mesos.hpp"

#include "process/help.hpp"
#include "process/http.hpp"
#include "process/future.hpp"
#include "process/limiter.hpp"
#include "process/process.hpp"

#include "stout/lambda.hpp"
#include "stout/synchronized.hpp"

#include "valve.hpp"

namespace mesos {
namespace serenity {

// TODO(bplotka): Break into explicit using-declarations.
using namespace process;  // NOLINT(build/namespaces)

using std::atomic_bool;
using std::string;

static const string ESTIMATOR_VALVE_ENDPOINT_HELP() {
  return HELP(
      TLDR(
          "Disable or enable Serenity Resource Estimator."),
      USAGE(
          VALVE_ROUTE),
      DESCRIPTION(
          "This endpoint is a valve for Serenity Resource Estimator. ",
          "When turned off it stops estimating new slack resources. ",
          "",
          "The following field should be supplied in a POST:",
          "1. " + PIPELINE_ENABLE_KEY + " - true / false."));
}


static const string CONTROLLER_VALVE_ENDPOINT_HELP() {
  return HELP(
      TLDR(
          "Disable or enable Serenity QoS Controller."),
      USAGE(
          VALVE_ROUTE),
      DESCRIPTION(
          "This endpoint is a valve for QoS Controller. ",
          "WARINIG: When turned off it stops assuring QoS for ",
          "Oversubscription feature - Best Effort executors can possibly",
          "impact Production executors. ",
          "Otherwise QoS Controller will assure QoS, by preemption of BE ",
          "tasks. ",
          "",
          "The following field should be supplied in a POST:",
          "1. " + PIPELINE_ENABLE_KEY + " - true / false."));
}


Try<string> getFormValue(
    const string& key,
    const hashmap<string, string>& values) {
  Option<string> value = values.get(key);

  if (value.isNone()) {
    return Error("Missing value for '" + key + "'.");
  }

  // HTTP decode the value.
  Try<string> decodedValue = process::http::decode(value.get());
  if (decodedValue.isError()) {
    return decodedValue;
  }

  // Treat empty string as an error.
  if (decodedValue.isSome() && decodedValue.get().empty()) {
    return Error("Empty string for '" + key + "'.");
  }

  return decodedValue.get();
}

template<class Type>
class ValveFilterEndpointProcess
  : public Process<ValveFilterEndpointProcess<Type>> {
 public:
  explicit ValveFilterEndpointProcess(bool _opened)
    : ProcessBase(getValveProcessBaseName<Type>()),
      opened(_opened),
      limiter(2, Seconds(1)) {}  // 2 permits per second.

  virtual ~ValveFilterEndpointProcess() {}

  void setOpen(bool open) {
    // NOTE: In future we may want to trigger some actions here.
    switch (Type::type) {
      case RESOURCE_ESTIMATOR:
        LOG(INFO) << VALVE_FILTER_NAME
                  << (open?"Enabling":"Disabling") << " slack estimations.";
        break;
      case QOS_CONTROLLER:
        LOG(INFO) << VALVE_FILTER_NAME
                  << (open?"Enabling":"Disabling") << " QoS assurance.";
        break;
    }
    this->opened = open;
  }

  Future<bool> isOpened() {
    return this->opened;
  }

  const lambda::function<Future<bool>()> getIsOpenedFunction() {
    return defer(this->self(),
                 &ValveFilterEndpointProcess<Type>::Self::isOpened);
  }

 protected:
  virtual void initialize() {
    switch (Type::type) {
      case RESOURCE_ESTIMATOR:
        this->route(VALVE_ROUTE,
              ESTIMATOR_VALVE_ENDPOINT_HELP(),
              &ValveFilterEndpointProcess<Type>::valve);
        LOG(INFO) << VALVE_FILTER_NAME
                  << "Resource Estimator endpoint initialized "
                  << "on /" << RESOURCE_ESTIMATOR_VALVE_PROCESS_BASE
                  << VALVE_ROUTE;
        break;
      case QOS_CONTROLLER:
        this->route(VALVE_ROUTE,
              CONTROLLER_VALVE_ENDPOINT_HELP(),
              &ValveFilterEndpointProcess<Type>::valve);
        LOG(INFO) << VALVE_FILTER_NAME
                  << "QoS Controller endpoint initialized "
                  << "on /" << QOS_CONTROLLER_VALVE_PROCESS_BASE
                  << VALVE_ROUTE;
        break;
    }
  }

 private:
  Future<http::Response> valve(const http::Request& request) {
    return limiter.acquire()
      .then(defer(this->self(),
                  &ValveFilterEndpointProcess<Type>::Self::_valve, request));
  }

  Future<http::Response> _valve(const http::Request& request) {
    Try<hashmap<string, string>> decode =
        process::http::query::decode(request.body);
    if (decode.isError()) {
      return http::BadRequest(
          std::string(VALVE_FILTER_NAME) + "Unable to decode query string: "
          + decode.error());
    }
    hashmap<string, string> values = decode.get();

    // Get params.
    string enabled_param;
    Option<string> pipeline_enable = request.query.get(PIPELINE_ENABLE_KEY);
    if (pipeline_enable.isSome()) {
      enabled_param = pipeline_enable.get();
    } else {
      Try<string> pipeline_enable_form =
          getFormValue(PIPELINE_ENABLE_KEY, values);

      if (pipeline_enable_form.isError()) {
        return http::BadRequest(pipeline_enable_form.error());
      }
      enabled_param = pipeline_enable_form.get();
    }

    bool pipeline_enable_decision = false;
    if (!enabled_param.compare("true")) {
      pipeline_enable_decision = true;
    }

    this->setOpen(pipeline_enable_decision);

    JSON::Object response;
    response.values[PIPELINE_ENABLE_KEY] = enabled_param;

    return http::OK(response);
  }

  atomic_bool opened;
  //! Used to rate limit the endpoint.
  RateLimiter limiter;
};


template<class Type>
ValveFilter<Type>::ValveFilter(bool _opened)
  : process(new ValveFilterEndpointProcess<Type>(_opened)) {
  isOpened = process.get()->getIsOpenedFunction();
  spawn(process.get());
}


template<class Type>
ValveFilter<Type>::ValveFilter(
    Consumer<ResourceUsage>* _consumer,
    bool _opened)
  : Producer<ResourceUsage>(_consumer),
    process(new ValveFilterEndpointProcess<Type>(_opened)) {
  isOpened = process.get()->getIsOpenedFunction();
  spawn(process.get());
}


template<class Type>
ValveFilter<Type>::~ValveFilter() {
  terminate(process.get());
  wait(process.get());
}


template<class Type>
Try<Nothing> ValveFilter<Type>::consume(const ResourceUsage& in) {
  if (this->isOpened().get()) {
    this->produce(in);
  } else {
    // Currently we are not continuing pipeline in case of closed valve.
    LOG(INFO) << VALVE_FILTER_NAME
              << "pipeline is closed";
  }

  return Nothing();
}

// Fix for using templated methods in .cpp file.
// See:
//    https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template class ValveFilter<Estimator>;
template class ValveFilter<QoS>;

}  // namespace serenity
}  // namespace mesos
