#include "bus/event_bus.hpp"

namespace mesos {
namespace serenity {

std::once_flag EventBus::onlyOneInit;
std::shared_ptr<EventBus> EventBus::instance = nullptr;

}  // namespace serenity
}  // namespace mesos
