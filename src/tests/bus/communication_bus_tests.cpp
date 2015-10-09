#include "bus/communication_bus.hpp"

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "messages/serenity.hpp"

#include "mesos/mesos.hpp"

#include "process/gtest.hpp"

#include "tests/common/sinks/dummy_sink.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using ::testing::_;
using ::testing::Return;

class BusTester : public Consumer<OversubscriptionControlMessage>,
                  public Producer<OversubscriptionControlMessage> {
 public:
  BusTester() {}

  Try<Nothing> consume(const OversubscriptionControlMessage& in) {
    return Nothing();
  }


  void sendOversubsciptionCtlMessage(bool ctl) {
    OversubscriptionControlMessage msg;
    msg.set_enableoversubscription(ctl);
    Producer<OversubscriptionControlMessage>::produce(msg);
  }


  template <typename T>
  Try<Nothing> addConsumer(Consumer<T>* consumer) {
    return Producer<T>::addConsumer(consumer);
  }
};


class BusTesterMock : public BusTester {
 public:
  MOCK_METHOD1(consume, Try<Nothing>(
      const OversubscriptionControlMessage& in));
};


ACTION_P(CheckOversub, check) {
  OversubscriptionControlMessage msg = arg0;
  EXPECT_EQ(msg.enableoversubscription(), check);
  return Nothing();
}


/**
 * Test of pipeline communication.
 * Component in one pipline sends msg to another pipeline.
 * Flow:
 * msg_sender -> qos_bus -> ru_bus -> msg_receiver
 */
TEST(CommunicationBusTests, SendMessageBetweenBus) {
  CommunicationBus ruBus(CommunicationBus::RE_BUS);
  CommunicationBus qosBus(CommunicationBus::QOS_BUS);

  BusTester msg_sender;
  BusTesterMock msg_receiver;

  msg_sender.addConsumer<OversubscriptionControlMessage>(&qosBus);
  ruBus.addConsumer<OversubscriptionControlMessage>(&msg_receiver);

  process::spawn(ruBus);
  process::spawn(qosBus);
  sleep(1);

  EXPECT_CALL(msg_receiver, consume(_)).Times(4)
      .WillOnce(CheckOversub(true))
      .WillOnce(CheckOversub(false))
      .WillOnce(CheckOversub(true))
      .WillOnce(CheckOversub(false));
  msg_sender.sendOversubsciptionCtlMessage(true);
  msg_sender.sendOversubsciptionCtlMessage(false);
  msg_sender.sendOversubsciptionCtlMessage(true);
  msg_sender.sendOversubsciptionCtlMessage(false);

  sleep(1);

  process::terminate(ruBus.self(), false);
  process::terminate(qosBus.self(), false);
  process::wait(ruBus.self());
  process::wait(qosBus.self());
}


TEST(CommunicationBusTests, GetBusUPID) {
  CommunicationBus ruBus(CommunicationBus::RE_BUS);

  process::spawn(ruBus);
  sleep(1);

  ASSERT_TRUE(CommunicationBus::getBusUPID(CommunicationBus::RE_BUS).isSome());
  ASSERT_TRUE(CommunicationBus::getBusUPID("NotABus").isNone());

  process::terminate(ruBus.self(), false);
  process::wait(ruBus.self());
}


}  // namespace tests
}  // namespace serenity
}  // namespace mesos

