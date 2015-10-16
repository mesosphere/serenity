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
 * Helper snippet of code for internal Envelope message type evaluation.
 *
 * Each event defined in proto need to be wrapped in "Envelope"
 * with message variable e.g:
 *
 * message SomeEventEnvelope {
 *  message SomeEvent {
 *   optional bool first_event_param = 1;
 *   optional bool second_event_param = 2;
 *   (...)
 *  }
 *
 *  optional SomeEvent message = 1;
 * }
 */
template <class Envelope>
using MessageType =
  typename std::remove_const<
    typename std::remove_reference<decltype(std::declval<Envelope>().message())
    >::type
  >::type;


/**
 *  Lookup Event-based bus made as libprocess actor.
 *  Used as singleton.
 */
class EventBus : public ProtobufProcess<EventBus> {
 public:
  /**
   * Creates a new instance of the EventBus if hasn't already been created.
   * Returns the singleton instance.
   *
   * Using call_once to ensure multi-threading support.
   */
  static std::shared_ptr<EventBus> GetInstance() {
    std::call_once(EventBus::onlyOneInit,
      []() {
        EventBus::instance.reset(new EventBus());
        process::spawn(*EventBus::instance);
      });

    return EventBus::instance;
  }

  /**
   * Subscribe for a specific Envelope Type.
   * TODO(bplotka): We can add here additional topic param and implement
   * topic lookup in publish.
   */
  template <typename T>
  static Try<Nothing> subscribe(process::UPID _subscriberPID) {
    EventBus::GetInstance()->_subscribe<T>(_subscriberPID);
    return Nothing();
  }

  /**
   * Publish Envelope.
   */
  template <typename T>
  static Try<Nothing> publish(const T& in) {
    EventBus::GetInstance()->_publish<T>(in);
    return Nothing();
  }

  ~EventBus() {
    if (EventBus::instance != nullptr) {
      process::terminate(EventBus::instance->self());
      process::wait(EventBus::instance->self());

      EventBus::instance = nullptr;
    }
  }

 private:
  // Private constructor.
  EventBus() {}

  // Disable copying.
  explicit EventBus(EventBus&) = delete;

  // Disable copying.
  void operator=(EventBus&) = delete;

  /**
   * Publishing message.
   *
   * Thread safe.
   */
  template <typename T>
  Try<Nothing> _publish(const T& in) {
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
  Try<Nothing> _subscribe(process::UPID _subscriberPID) {
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

  static std::once_flag onlyOneInit;

  // Singleton class instance.
  static std::shared_ptr<EventBus> instance;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EVENT_BUS_HPP
