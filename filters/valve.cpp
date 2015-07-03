#include <string>

#include "glog/logging.h"

#include "process/help.hpp"
#include "process/http.hpp"
#include "process/future.hpp"
#include "process/limiter.hpp"
#include "process/process.hpp"
#include "process/mutex.hpp"

#include "stout/lambda.hpp"
#include "stout/synchronized.hpp"

#include "valve.hpp"

namespace mesos {
namespace serenity {

// TODO(bplotka): Break into explicit using-declarations.
using namespace process;  // NOLINT(build/namespaces)

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


class ValveFilterEndpointProcess : public Process<ValveFilterEndpointProcess> {
 public:
  ValveFilterEndpointProcess(ValveType _valveType, bool _opened)
    : ProcessBase(getProcessBaseName(_valveType)),
      valveType(_valveType),
      opened(_opened),
      limiter(2, Seconds(1)) {}  // 2 permits per second.

  virtual ~ValveFilterEndpointProcess() {}

  void safeOpen(bool open) {
    synchronized(control_mutex) {
      // NOTE: In future we may want to trigger some actions here.
      this->opened = open;
    }
  }

  Future<bool> isOpened() {
    return this->opened;
  }

  const lambda::function<Future<bool>()> getIsOpenedFunction() {
    return defer(self(), &Self::isOpened);
  }

 protected:
  virtual void initialize() {
    switch (valveType) {
      case RESOURCE_ESTIMATOR_VALVE:
        route(VALVE_ROUTE,
              ESTIMATOR_VALVE_ENDPOINT_HELP(),
              &ValveFilterEndpointProcess::valve);
        LOG(INFO) << "Serenity Resource Estimator endpoint initialized "
                  << "on /" << RESOURCE_ESTIMATOR_PROCESS_BASE  << VALVE_ROUTE;
        break;
      case QOS_CONTROLLER_VALVE:
        route(VALVE_ROUTE,
              CONTROLLER_VALVE_ENDPOINT_HELP(),
              &ValveFilterEndpointProcess::valve);
        LOG(INFO) << "Serenity QoS Controller endpoint initialized "
                  << "on /" << QOS_CONTROLLER_PROCESS_BASE  << VALVE_ROUTE;
        break;
    }
  }

 private:
  Future<http::Response> valve(const http::Request& request) {
    return limiter.acquire()
      .then(defer(self(), &Self::_valve, request));
  }

  Future<http::Response> _valve(const http::Request& request) {
    Try<hashmap<string, string>> decode =
        process::http::query::decode(request.body);
    if (decode.isError()) {
      return http::BadRequest(
          "Unable to decode query string: " + decode.error());
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

    this->safeOpen(pipeline_enable_decision);

    JSON::Object response;
    response.values[PIPELINE_ENABLE_KEY] = enabled_param;

    return http::OK(response);
  }

  bool opened;
  Mutex control_mutex;
  ValveType valveType;
  //! Used to rate limit the endpoint.
  RateLimiter limiter;
};


ValveFilter::ValveFilter(
    Consumer<ResourceUsage>* _consumer,
    ValveType _valveType,
    bool _opened)
  : Producer<ResourceUsage>(_consumer),
    process(new ValveFilterEndpointProcess(_valveType, _opened)) {
  isOpened = process.get()->getIsOpenedFunction();
  spawn(process.get());
}


ValveFilter::~ValveFilter() {
  terminate(process.get());
  wait(process.get());
}


Try<Nothing> ValveFilter::consume(const ResourceUsage& in) {
  if (this->isOpened().get()) {
    this->produce(in);
  }

  // TODO(bplotka): Should we continue the pipeline without any Executor in
  // usage?
  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
