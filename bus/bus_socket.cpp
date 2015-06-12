#include <stout/nothing.hpp>
#include <stout/try.hpp>

#include "serenity/serenity.hpp"


namespace mesos {
namespace serenity {


Try<Nothing> BusSocket::registration(std::string topic){
  return Nothing();
}


//skonefal TODO: Waiting for serenity.proto generic serenity event
Try<Nothing> BusSocket::subscribe(
    std::string topic,
    std::function<void(mesos::scheduler::Event)> callback) {
  return Nothing();
}


//skonefal TODO: Waiting for serenity.proto generic serenity event
Try<Nothing> BusSocket::publish(std::string topic, mesos::scheduler::Event event){
  return  Nothing();
}

} // namespace serenity
} // namespace mesos
