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
  TestEventConsumer(bool enabled)
    : ProtobufProcess(),
      oversubscription_enabled(enabled)  {
    install<OversubscriptionControlEventEnvelope>(
      &TestEventConsumer::event,
      &OversubscriptionControlEventEnvelope::message);
  }

  bool oversubscription_enabled;

  void event(const EventType<OversubscriptionControlEventEnvelope>& msg) {
    LOG(INFO) << "Got Message!";
    this->oversubscription_enabled = msg.enable_oversubscription();
  }
};

TEST(EventBus, SubscribeAndPublish) {

  // Create consumer with endpoint installed.
  TestEventConsumer consumer(false);
  process::spawn(consumer);

  // Subscribe for OversubscriptionControlEvent messages.
  EventBus::subscribe<OversubscriptionControlEventEnvelope>(consumer.self());

  // Prepare message to enable oversubscription.
  OversubscriptionControlEventEnvelope envelope;
  envelope.mutable_message()->set_enable_oversubscription(true);
  EventBus::publish<OversubscriptionControlEventEnvelope>(envelope);

  // Wait for libprocess queue to be processed.
  process::Clock::pause();
  process::Clock::settle();

  EXPECT_TRUE(consumer.oversubscription_enabled);

  // Disable oversubscription.
  envelope.mutable_message()->set_enable_oversubscription(false);
  EventBus::publish<OversubscriptionControlEventEnvelope>(envelope);

  // Wait for libprocess queue to be processed.
  process::Clock::pause();
  process::Clock::settle();

  EXPECT_FALSE(consumer.oversubscription_enabled);

  // Release libprocess threads.
  EventBus::Release();
  process::terminate(consumer, false);
  process::wait(consumer);
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

