#include "bus/event_bus.hpp"

#include "filters/valve.hpp"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "glog/logging.h"

#include "messages/serenity.hpp"

#include "mesos/mesos.hpp"

#include "process/clock.hpp"
#include "process/gtest.hpp"
#include "process/process.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using ::testing::_;
using ::testing::Return;

class TestEventConsumer :
  public ProtobufProcess<TestEventConsumer> {
 public:
  explicit TestEventConsumer(bool enabled)
    : ProtobufProcess(),
      oversubscription_enabled(enabled)  {
    install<OversubscriptionCtrlEventEnvelope>(
      &TestEventConsumer::event,
      &OversubscriptionCtrlEventEnvelope::message);
  }

  bool oversubscription_enabled;

  void event(const OversubscriptionCtrlEvent& msg) {
    LOG(INFO) << "Got Message!";
    this->oversubscription_enabled = msg.enable();
  }
};

/**
 * Subscribe for an OversubscriptionCtrlEventEnvelope and check if published event
 * of that type will be receive by subscriber.
 */
TEST(EventBus, SubscribeAndPublish) {
  // Create consumer with endpoint installed.
  TestEventConsumer consumer(false);
  process::spawn(consumer);

  // Subscribe for OversubscriptionCtrlEventEnvelope messages.
  StaticEventBus::subscribe<OversubscriptionCtrlEventEnvelope>(consumer.self());

  // Prepare message to enable oversubscription.
  OversubscriptionCtrlEventEnvelope envelope;
  envelope.mutable_message()->set_enable(true);
  StaticEventBus::publish<OversubscriptionCtrlEventEnvelope>(envelope);

  // Wait for libprocess queue to be processed.
  process::Clock::pause();
  process::Clock::settle();

  EXPECT_TRUE(consumer.oversubscription_enabled);

  // Disable oversubscription.
  envelope.mutable_message()->set_enable(false);
  StaticEventBus::publish<OversubscriptionCtrlEventEnvelope>(envelope);

  // Wait for libprocess queue to be processed.
  process::Clock::pause();
  process::Clock::settle();

  EXPECT_FALSE(consumer.oversubscription_enabled);

  // Clear Clock.
  process::Clock::resume();

  // Release libprocess threads.
  process::terminate(consumer);
  process::wait(consumer);
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

