#include "bus/event_bus.hpp"

namespace mesos {
namespace serenity {

std::once_flag StaticEventBus::onlyOneEventBusInit;
std::unique_ptr<EventBus> StaticEventBus::eventBus = nullptr;

}  // namespace serenity
}  // namespace mesos
