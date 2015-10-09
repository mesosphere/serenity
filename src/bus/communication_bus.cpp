#include <map>
#include <string>
#include <mutex>  // NOLINT [build/c++11]

#include "bus/communication_bus.hpp"

namespace mesos {
namespace serenity {

/** Static functions */

Option<process::UPID> CommunicationBus::getBusUPID(
    const std::string& _busName) {
  std::lock_guard<std::mutex> lock(busMapLock);
  return CommunicationBus::_getBusUPID(_busName);
}


Option<process::UPID> CommunicationBus::_getBusUPID(
    const std::string& _busName) {
  if (0 == CommunicationBus::busMap.count(_busName)) {
    return None();
  } else {
    return CommunicationBus::busMap[_busName];
  }
}


/** Member functions */

Try<Nothing> CommunicationBus::registerBus(const std::string& _busName,
                                           const process::UPID& _busUPID) {
  std::lock_guard<std::mutex> lock(busMapLock);
  busMap[_busName] = _busUPID;
  return Nothing();
}


std::mutex CommunicationBus::busMapLock;
std::map<std::string, process::UPID> CommunicationBus::busMap;
const std::string CommunicationBus::RE_BUS = "RE";
const std::string CommunicationBus::QOS_BUS = "QoS";

}  // namespace serenity
}  // namespace mesos
