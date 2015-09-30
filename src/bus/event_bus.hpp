#ifndef SERENITY_EVENT_BUS_HPP
#define SERENITY_EVENT_BUS_HPP

#include <map>
// TODO(skonefal): Move to std shared_mutex when -std=c++14 is available
#include <mutex>  // NOLINT [build/c++11]
#include <string>
#include <typeinfo>

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

template <class T, class M> M get_member_type(M T:: *);
#define GET_TYPE_OF(mem) decltype(get_member_type(mem))


/**
 *  Lookup Event based bus made as libprocess actor.
 *  Singleton.
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
    }
    return EventBus::instance;
  }

  template <typename T>
  static Try<Nothing> subscribe(process::UPID _subscriberPID) {
    EventBus::GetInstance()->_subscribe<T>(_subscriberPID);
    return Nothing();
  }

  template <typename T>
  static Try<Nothing> registerEvent() {
    EventBus::GetInstance()->_registerEvent<T>();
    return Nothing();
  }

  template <typename T>
  void receiveMsg(const T& msg) {

  }

  template <typename T>
  Try<Nothing> _registerEvent() {
    //GET_TYPE_OF(&T::message) x;
    //x.set_value(true);
//    install<T>(&EventBus::receiveMsg<GET_TYPE_OF(&T::message)>,
//               &T::message);

    return Nothing();
  }

  /**
   * Currently topic == type of message (T)
   */
  template <typename T>
  Try<Nothing> _subscribe(process::UPID _subscriberPID) {
    std::lock_guard<std::mutex> lock(typeMapLock);
    typeMap[typeid(T::message)] = _subscriberPID;
    return Nothing();

  }
  /**
   * Mutex for locking topicMap
   * TODO(skonefal): remove when c++14 & shared_mutex implemented
   */
  std::mutex typeMapLock;
  std::map<std::type_index, process::UPID> typeMap;

  std::mutex registrationListLock;
  std::list<std::type_index> registrationList;

  // Singleton class instance
  static EventBus* instance;
};

//   */
//  template <typename T>
//  Try<Nothing> sendMessage(const T& _msg) {
//    LOG(INFO) << this->tag << ": Broadcasting message";
//    std::lock_guard<std::mutex> lock(busMapLock);
//    for (const auto& pid : CommunicationBus::busMap) {
//      send(pid.second, _msg);
//    }
//    return Nothing();
//  }
//
//
//  /**
//   * Sends message to _to UPID
//   */
//  template <typename T>
//  Try<Nothing> sendMessage(const process::UPID& _to, const T& _msg) {
//    LOG(INFO) << this->tag << "Sending message to " << _to;
//    send(_to, _msg);
//    return Nothing();
//  }
//
//
//  /**
//   * Sends message to _busname bus
//   */
//  template <typename T>
//  Try<Nothing> sendMessage(const std::string& _busName, const T& _msg) {
//    LOG(INFO) << this->tag << "Sending message to " << _busName;
//    std::lock_guard<std::mutex> lock(busMapLock);
//    Option<process::UPID> busUPID = CommunicationBus::_getBusUPID(_busName);
//    if (busUPID.isSome()) {
//      send(busUPID.get(), _msg);
//      return Nothing();
//    } else {
//      return Error("Cannot find bus with name " + _busName);
//    }
//  }
//
//
//  /**
//   * Consume interface
//   */
//  Try<Nothing> consume(const OversubscriptionControlMessage& in) {
//    LOG(INFO) << this->tag << ": Consuming OversubscriptionControlMessage";
//    OversubscriptionControlMessageEnvelope msg;
//    msg.set_allocated_oversubscription_control_message(
//        new OversubscriptionControlMessage(in));
//    return this->sendMessage(msg);
//  }
//
//
//  /** Produce interface */
//  template <typename T>
//  Try<Nothing> addConsumer(Consumer<T>* consumer) {
//    LOG(INFO) << this->tag << ": Addind consumer...";
//    return Producer<T>::addConsumer(consumer);
//  }
//
//
//  // Default bus names
//  static const std::string RE_BUS;
//  static const std::string QOS_BUS;
//
// protected:
//  /**
//  * Registration for other buses that would like to receive
//  * messages from Serenity modules
//  */
//  static Try<Nothing> registerBus(const std::string& _busName,
//                                  const process::UPID& _busUPID);
//
//
//  /**
//   * Returns bus UPID from bus name.
//   * Does not use busMapLock mutex and operates on busMap structure.
//   * TODO(skonefal): remove when c++14 & shared_mutex implemented
//   */
//  static Option<process::UPID> _getBusUPID(const std::string& _busName);
//
//
//  /**
//  * Callback for receving messages from other queues
//  */
//  template <typename T>
//  void receiveMsg(const T& msg) {
//    Producer<T>::produce(msg);
//  }
//
//
//  const std::string tag;
//
//
//};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EVENT_BUS_HPP
