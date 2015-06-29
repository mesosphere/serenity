#ifndef SERENITY_TEST_MOCK_SINK_HPP
#define SERENITY_TEST_MOCK_SINK_HPP

#include <process/gmock.hpp>
#include <process/gtest.hpp>

#include <stout/try.hpp>

#include "serenity/serenity.hpp"

#include "tests/common/usage_helper.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::DoDefault;
using ::testing::Return;

namespace mesos {
namespace serenity {
namespace tests {

template<typename T>
class MockSink : public Consumer<T> {
 public:
  MockSink() : numberOfMessagesConsumed(0) {
    ON_CALL(*this, consume(_))
      .WillByDefault(InvokeConsume(this));
    EXPECT_CALL(*this, consume(_))
      .WillRepeatedly(DoDefault());
  }

  // TODO(bplotka): In future we can move it
  // to the EMATest Gtest Fixture class.
  void expectIpc(
      int32_t usage_index, double_t value, double_t threshold) {
    ASSERT_TRUE(
        currentConsumedUsage.executors().size() > usage_index);

    ResourceStatistics stats =
      currentConsumedUsage.executors(usage_index).statistics();

    EXPECT_TRUE(stats.has_net_tcp_active_connections());

    EXPECT_NEAR(
        stats.net_tcp_active_connections(), value, threshold);
  }

  // TODO(bplotka): In future we can move it
  // to the EMATest Gtest Fixture class.
  void expectCpuUsage(
      int32_t usage_index, double_t value, double_t threshold) {
    ASSERT_TRUE(
        currentConsumedUsage.executors().size() > usage_index);

    ResourceStatistics stats =
      currentConsumedUsage.executors(usage_index).statistics();

    EXPECT_TRUE(stats.has_net_tcp_time_wait_connections());

    EXPECT_NEAR(
        stats.net_tcp_time_wait_connections(), value, threshold);
  }

  virtual ~MockSink() {}

  int numberOfMessagesConsumed;
  ResourceUsage currentConsumedUsage;

  MOCK_METHOD1_T(consume, Try<Nothing>(const T& in));
};


ACTION_P(InvokeConsume, sink) {
  sink->numberOfMessagesConsumed++;
  return Nothing();
}


ACTION_P(InvokeConsumeUsage, sink) {
  sink->numberOfMessagesConsumed++;
  sink->currentConsumedUsage.CopyFrom(arg0);
  return Nothing();
}

// For debug only.
ACTION_P(InvokeConsumePrintIpcEma, sink) {
  sink->numberOfMessagesConsumed++;
  std::cout << "Received ResourceUsage. Excutors num: "
            << arg0.executors().size() << std::endl;
  foreach(ResourceUsage_Executor inExec, arg0.executors()) {
    if (!inExec.statistics().has_net_tcp_active_connections()) {
      std::cout << "Does not have IPC value" << std::endl;
      continue;
    }

    std::cout <<
    "Executor(" <<
    inExec.executor_info().executor_id().value() <<
    ") EMA IPC value: " <<
    inExec.statistics().net_tcp_active_connections() <<
    std::endl;
  }

  return Nothing();
}


// For debug only.
ACTION_P(InvokeConsumePrintCPuUsageEma, sink) {
  sink->numberOfMessagesConsumed++;
  std::cout << "Received ResourceUsage. Excutors num: "
            << arg0.executors().size() << std::endl;
  foreach(ResourceUsage_Executor inExec, arg0.executors()) {
    if (!inExec.statistics().has_net_tcp_time_wait_connections()) {
      std::cout << "Does not have IPC value" << std::endl;
      continue;
    }
    std::cout <<
    "Executor(" <<
    inExec.executor_info().executor_id().value() <<
    ") EMA Cpu Usage value: " <<
    inExec.statistics().net_tcp_time_wait_connections() <<
    std::endl;
  }
  return Nothing();
}


}  // namespace tests
}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_TEST_MOCK_SINK_HPP
