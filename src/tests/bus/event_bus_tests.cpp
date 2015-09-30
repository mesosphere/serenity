#include "bus/event_bus.hpp"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "messages/serenity.hpp"

#include "mesos/mesos.hpp"

#include "process/gtest.hpp"


namespace mesos {
namespace serenity {
namespace tests {

using ::testing::_;
using ::testing::Return;


TEST(EventBus, SubscribeAndPublish) {
  EventBus::registerEvent<OversubscriptionControlEventEnvelope>();

  process::terminate(EventBus::GetInstance()->self(), false);
  process::wait(EventBus::GetInstance()->self());
}



}  // namespace tests
}  // namespace serenity
}  // namespace mesos

