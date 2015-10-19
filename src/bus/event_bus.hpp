#ifndef SERENITY_EVENT_BUS_HPP
#define SERENITY_EVENT_BUS_HPP

#include <map>
#include <unordered_set>
#include <memory>
// TODO(skonefal): Move to std shared_mutex when -std=c++14 is available
#include <mutex>  // NOLINT [build/c++11]
#include <string>
#include <type_traits>
#include <typeinfo>
#include <typeindex>

#include "glog/logging.h"

#include "messages/serenity.hpp"

#include "process/process.hpp"
#include "process/protobuf.hpp"

#include "serenity/serenity.hpp"

#include "stout/error.hpp"
#include "stout/nothing.hpp"
#include "stout/option.hpp"

namespace mesos {
namespace serenity {


/**
 * Event definition.
 *
 * Each event defined in proto need to be wrapped in "Envelope"
 * with message variable e.g:
 *
 * message SomeEvent {
 *   optional bool first_event_param = 1;
 *   optional bool second_event_param = 2;
 *   (...)
 * }
 *
 * message SomeEventEnvelope {
 *   optional SomeEvent message = 1;
 * }
 */


/**
 *  Lookup Event-based bus made as libprocess actor.
 */
class EventBus : public ProtobufProcess<EventBus> {
 public:
  EventBus() {
    process::spawn(this);
  }

  /**
   * Publishing message.
   *
   * Thread safe.
   */
  template <typename T>
  Try<Nothing> publish(const T& in) {
    std::unordered_set<process::UPID> subscribers;
    {
      // Synchronized block of code.
      std::lock_guard<std::mutex> lock(subscribersMapLock);

      auto subscribersForType = this->subscribersMap.find(typeid(T));
      if (subscribersForType == this->subscribersMap.end()) {
        // Nobody subscribed for this event.
        LOG(INFO) << "Nobody subscribed for this event.";
        return Nothing();
      }

      subscribers = subscribersForType->second;
    }

    for (const process::UPID& subscriberPID : subscribers) {
      LOG(INFO) << "Sending to: " << subscriberPID;
      T msg(in);
      this->send(subscriberPID, msg);
    }

    return Nothing();
  }

  /**
   * Register subscriber for particular type of Envelope.
   * Currently topic (classifier) is equal to T id.
   *
   * TODO(bplotka): We can add here additional topic param and implement
   * topic lookup in publish.
   *
   * Thread safe.
   */
  template <typename T>
  Try<Nothing> subscribe(process::UPID _subscriberPID) {
    std::lock_guard<std::mutex> lock(subscribersMapLock);

    auto subscribersForType = this->subscribersMap.find(typeid(T));
    if (subscribersForType == this->subscribersMap.end()) {
      // Nobody has subscribed for this event type before.
      std::unordered_set<process::UPID> subscribersSet;
      subscribersSet.insert(_subscriberPID);

      this->subscribersMap[typeid(T)] = subscribersSet;
      return Nothing();
    }

    if (subscribersForType->second.find(_subscriberPID) !=
        subscribersForType->second.end()) {
      LOG(INFO) << "This subscriber with PID: " << _subscriberPID
      << "is already registered";
      return Nothing();
    }
    subscribersForType->second.insert(_subscriberPID);

    return Nothing();
  }

  /**
   * Mutex for locking subscribersMap.
   * TODO: Remove when c++14 & shared_mutex implemented
   */
  std::mutex subscribersMapLock;
  std::map<std::type_index, std::unordered_set<process::UPID>> subscribersMap;
};


/**
 * Static wrapper for Event-based bus made to share state between pipelines
 * within same lib.
 */
class StaticEventBus {
 public:
  /**
   * Subscribe for a specific Envelope Type.
   * TODO(bplotka): We can add here additional topic param and implement
   * topic lookup in publish.
   */
  template <typename T>
  static Try<Nothing> subscribe(process::UPID _subscriberPID) {
    StaticEventBus::GetEventBus()->subscribe<T>(_subscriberPID);
    return Nothing();
  }

  /**
   * Publish Envelope.
   */
  template <typename T>
  static Try<Nothing> publish(const T& in) {
    StaticEventBus::GetEventBus()->publish<T>(in);
    return Nothing();
  }

  /**
   * Destructor.
   */
  ~StaticEventBus() {
    if (StaticEventBus::eventBus != nullptr) {
      process::terminate(StaticEventBus::eventBus->self());
      process::wait(StaticEventBus::eventBus->self());

      StaticEventBus::eventBus = nullptr;
    }
  }

 private:
  // Private constructor.
  StaticEventBus() {}

  // Disable copying.
  explicit StaticEventBus(StaticEventBus&) = delete;

  // Disable copying.
  void operator=(StaticEventBus&) = delete;


  static const std::unique_ptr<EventBus>& GetEventBus() {
    std::call_once(StaticEventBus::onlyOneEventBusInit,
       []() {
         StaticEventBus::eventBus.reset(new EventBus());
       });

    return StaticEventBus::eventBus;
  }

  static std::once_flag onlyOneEventBusInit;
  static std::unique_ptr<EventBus> eventBus;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EVENT_BUS_HPP
