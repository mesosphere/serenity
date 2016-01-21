#include <atomic>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <typeindex>

#include "bus/event_bus.hpp"

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

using std::atomic_int;
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

#define IS_OPENED_THRESHOLD 1

class ValveFilterEndpointProcess
  : public ProtobufProcess<ValveFilterEndpointProcess> {
 public:
  explicit ValveFilterEndpointProcess(const Tag& _tag, bool _opened)
    : tag(_tag),
      ProcessBase(getValveProcessBaseName(_tag.TYPE())),
      openedCounter(_opened ?  IS_OPENED_THRESHOLD : IS_OPENED_THRESHOLD - 1),
      // 2 permits per second.
      limiter(2, Seconds(1)) {
    switch (this->tag.TYPE()) {
      case RESOURCE_ESTIMATOR:
        install<OversubscriptionCtrlEventEnvelope>(
          &ValveFilterEndpointProcess::setOpenHandle,
          &OversubscriptionCtrlEventEnvelope::message);

        // Subscribe for OversubscriptionCtrlEventEnvelope messages.
        StaticEventBus::subscribe<OversubscriptionCtrlEventEnvelope>(self());
        break;
      default:
        break;
    }
  }

  virtual ~ValveFilterEndpointProcess() {}

  void setOpen(bool open) {
    // NOTE: In future we may want to trigger some actions here.
    SERENITY_LOG(INFO) << (open?"Enabling":"Disabling") << " " << tag.AIM();

    if (open) {
      ++openedCounter;
    } else {
      --openedCounter;
    }
  }

  void setOpenHandle(const OversubscriptionCtrlEvent& msg) {
    this->setOpen(msg.enable());
  }

  Future<bool> isOpened() {
    return (openedCounter >= IS_OPENED_THRESHOLD);
  }

  const lambda::function<Future<bool>()> getIsOpenedFunction() {
    return defer(self(), &Self::isOpened);
  }

 protected:
  virtual void initialize() {
    route(VALVE_ROUTE,
          (tag.TYPE() == RESOURCE_ESTIMATOR?
              ESTIMATOR_VALVE_ENDPOINT_HELP():
              CONTROLLER_VALVE_ENDPOINT_HELP()),
          &ValveFilterEndpointProcess::valve);
    SERENITY_LOG(INFO)
      << "endpoint initialized "
      << "on /" << getValveProcessBaseName(tag.TYPE())
      << VALVE_ROUTE;
  }

 private:
  const Tag tag;
  Future<http::Response> valve(const http::Request& request) {
    return limiter.acquire()
      .then(defer(self(), &Self::_valve, request));
  }

  Future<http::Response> _valve(const http::Request& request) {
    Try<hashmap<string, string>> decode =
        process::http::query::decode(request.body);
    if (decode.isError()) {
      const string message =
        tag.NAME() + "Unable to decode query string: " + decode.error();
      return http::BadRequest(message);
    }
    hashmap<string, string> values = decode.get();

    string enabled_param;
    Option<string> pipeline_enable = request.query.get(PIPELINE_ENABLE_KEY);

    if (pipeline_enable.isSome()) {
      enabled_param = pipeline_enable.get();
    } else {
      // Get params via POST method.
      Try<string> pipeline_enable_form =
          getFormValue(PIPELINE_ENABLE_KEY, values);

      if (pipeline_enable_form.isError()) {
        // There is no 'enabled' param.
        const string message =
          tag.NAME() + "Missed 'enabled' param.";
        return http::BadRequest(message);
      }
      enabled_param = pipeline_enable_form.get();
    }
    bool pipeline_enable_decision;
    string message = "";

    if (!enabled_param.compare("true")) {
      pipeline_enable_decision = true;
      message = tag.NAME() + "Enabled.";
    } else if (!enabled_param.compare("false")) {
      pipeline_enable_decision = false;
      message = tag.NAME() + "Disabled.";
    } else {
      // Unknown value of 'enabled' param.
      return http::BadRequest(tag.NAME() + "Unknown value of 'enabled' param. "
                              + "Possible values: true / false");
    }

    // Open Pipeline (increment counter)
    this->setOpen(pipeline_enable_decision);

    return http::OK(message);
  }

  // Since there could be a lot of potential operators we need counter.
  // If counter is >= IS_OPENED_THRESHOLD than valve will be opened.
  atomic_int openedCounter;

  //! Used to rate limit the endpoint.
  RateLimiter limiter;
};


ValveFilter::ValveFilter(bool _opened, const Tag& _tag)
  : process(new ValveFilterEndpointProcess(_tag, _opened)), tag(_tag) {
  isOpened = process.get()->getIsOpenedFunction();
  spawn(process.get());
}


ValveFilter::ValveFilter(
    Consumer<ResourceUsage>* _consumer,
    bool _opened,
    const Tag& _tag)
  : Producer<ResourceUsage>(_consumer),
    process(new ValveFilterEndpointProcess(_tag, _opened)),
    tag(_tag) {
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
  } else {
    // Currently we are not continuing pipeline in case of closed valve.
    SERENITY_LOG(INFO) << "Pipeline is closed";
  }

  return Nothing();
}


}  // namespace serenity
}  // namespace mesos
