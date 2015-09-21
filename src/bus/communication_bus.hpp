#ifndef SERENITY_COMMUNICATIONBUS_HPP
#define SERENITY_COMMUNICATIONBUS_HPP

#include <map>
// TODO(skonefal): Move to std shared_mutex when -std=c++14 is available
#include <mutex>  // NOLINT [build/c++11]
#include <string>

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


class CommunicationBus : public ProtobufProcess<CommunicationBus>,
                         public Consumer<OversubscriptionControlMessage>,
                         public Producer<OversubscriptionControlMessage> {
 public:
  explicit CommunicationBus(std::string _tag) :
      ProcessBase(process::ID::generate(_tag)),
      tag(_tag) {}


  void initialize() override {
    CommunicationBus::registerBus(this->tag, this->self());

    install<OversubscriptionControlMessage_>(
        &CommunicationBus::receiveMsg<OversubscriptionControlMessage>,
        &OversubscriptionControlMessage_::oversubscription_control_message);
  }

  /**
  * Get bus UPID from key (bus name string)
  */
  static Option<process::UPID> getBusUPID(const std::string& _busName);


  /**
   * Send message to every registered bus
   */
  template <typename T>
  Try<Nothing> sendMessage(const T& _msg) {
    LOG(INFO) << this->tag << ": Broadcasting message";
    std::lock_guard<std::mutex> lock(busMapLock);
    for (const auto& pid : CommunicationBus::busMap) {
      send(pid.second, _msg);
    }
    return Nothing();
  }


  /**
   * Sends message to _to UPID
   */
  template <typename T>
  Try<Nothing> sendMessage(const process::UPID& _to, const T& _msg) {
    LOG(INFO) << this->tag << "Sending message to " << _to;
    send(_to, _msg);
    return Nothing();
  }


  /**
   * Sends message to _busname bus
   */
  template <typename T>
  Try<Nothing> sendMessage(const std::string& _busName, const T& _msg) {
    LOG(INFO) << this->tag << "Sending message to " << _busName;
    std::lock_guard<std::mutex> lock(busMapLock);
    Option<process::UPID> busUPID = CommunicationBus::_getBusUPID(_busName);
    if (busUPID.isSome()) {
      send(busUPID.get(), _msg);
      return Nothing();
    } else {
      return Error("Cannot find bus with name " + _busName);
    }
  }


  /**
   * Consume interface
   */
  Try<Nothing> consume(const OversubscriptionControlMessage& in) {
    LOG(INFO) << this->tag << ": Consuming OversubscriptionControlMessage";
    OversubscriptionControlMessage_ msg;
    msg.set_allocated_oversubscription_control_message(
        new OversubscriptionControlMessage(in));
    return this->sendMessage(msg);
  }


  /** Produce interface */
  template <typename T>
  Try<Nothing> addConsumer(Consumer<T>* consumer) {
    LOG(INFO) << this->tag << ": Addind consumer...";
    return Producer<T>::addConsumer(consumer);
  }


  // Default bus names
  static const std::string RE_BUS;
  static const std::string QOS_BUS;

 protected:
  /**
  * Registration for other buses that would like to receive
  * messages from Serenity modules
  */
  static Try<Nothing> registerBus(const std::string& _busName,
                                  const process::UPID& _busUPID);


  /**
   * Returns bus UPID from bus name.
   * Does not use busMapLock mutex and operates on busMap structure.
   * TODO(skonefal): remove when c++14 & shared_mutex implemented
   */
  static Option<process::UPID> _getBusUPID(const std::string& _busName);


  /**
  * Callback for receving messages from other queues
  */
  template <typename T>
  void receiveMsg(const T& msg) {
    Producer<T>::produce(msg);
  }


  const std::string tag;

  /**
   * Mutex for locking busMap
   * TODO(skonefal): remove when c++14 & shared_mutex implemented
   */
  static std::mutex busMapLock;
  static std::map<std::string, process::UPID> busMap;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_COMMUNICATION_BUS_HPP
