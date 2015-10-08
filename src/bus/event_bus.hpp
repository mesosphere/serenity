#ifndef SERENITY_EVENT_BUS_HPP
#define SERENITY_EVENT_BUS_HPP

#include <map>
#include <unordered_set>
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
 * message OversubscriptionControlEventEnvelope {
 *  message OversubscriptionControlEvent {
 *   optional bool enable_oversubscription = 1;
 *   (...)
 *  }
 *
 *  optional OversubscriptionControlEvent message = 1;
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
  virtual ~EventBus() { }

  /**
   * Creates a new instance of the EventBus if hasn't already been created.
   * Returns the singleton instance.
   */
  static EventBus* const GetInstance() {
    if (EventBus::instance == nullptr) {
      EventBus::instance = new EventBus();
      process::spawn(EventBus::instance);
    }
    return EventBus::instance;
  }

  /**
   * Releases Event Bus process. Responsibility is for the user side.
   */
  static Try<Nothing> const Release() {
    if (EventBus::instance != nullptr) {
      process::terminate(EventBus::instance->self());
      process::wait(EventBus::instance->self());

      delete(EventBus::instance);
      EventBus::instance = nullptr;
    }
    return Nothing();
  }

  /**
   * Returns UPID of Event Bus.
   */
  static process::UPID address() {
    return EventBus::GetInstance()->self();
  }

  /**
   * Subscribe for a specific Envelope Type.
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

 private:
  template <typename T>
  Try<Nothing> _publish(const T& in) {
    // Lock map?
    auto subscribersForType = this->subscribersMap.find(typeid(MessageType<T>));
    if (subscribersForType == this->subscribersMap.end()) {
      // Nobody subscribed for this event.
      LOG(INFO) << "Nobody subscribed for this event.";
      return Nothing();
    }

    for (const process::UPID& subscriberPID : subscribersForType->second) {
      LOG(INFO) << "Sending to: " << subscriberPID;
      T msg(in);
      this->send(subscriberPID, msg);
    }

    return Nothing();
  }

  /**
   * Register subscriber for particular type of Envelope.
   * Currently topic (classifier) is equal to MessageType<T> id.
   */
  template <typename T>
  Try<Nothing> _subscribe(process::UPID _subscriberPID) {
    std::lock_guard<std::mutex> lock(subscribersMapLock);

    auto subscribersForType = this->subscribersMap.find(typeid(MessageType<T>));
    if (subscribersForType == this->subscribersMap.end()) {
      // Nobody has subscribed for this event type before.

      std::unordered_set<process::UPID> subscribersSet;
      subscribersSet.insert(_subscriberPID);

      this->subscribersMap[typeid(MessageType<T>)] = subscribersSet;
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
   * Mutex for locking subscribersMap
   * TODO: Remove when c++14 & shared_mutex implemented
   */
  std::mutex subscribersMapLock;
  std::map<std::type_index, std::unordered_set<process::UPID>> subscribersMap;

  // Singleton class instance
  static EventBus* instance;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EVENT_BUS_HPP
