#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "mesos/mesos.hpp"

#include "filters/ignore_new_executors.hpp"

#include "tests/common/serenity.hpp"
#include "tests/common/sinks/dummy_sink.hpp"
#include "tests/common/sources/json_source.hpp"

#include "tests/common/sinks/printer_sink.hpp"
//#include "tests/filters/ignore_new_tasks_test.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using testing::Return;
using testing::Sequence;

constexpr int IGNORE_NEW_EXECUTORS_SAMPLES = 6;

class MockIgnoreNewExecutorsFilter : public IgnoreNewExecutorsFilter {
public:
  MockIgnoreNewExecutorsFilter(uint32_t arg) : IgnoreNewExecutorsFilter(arg) {}

  MOCK_METHOD1(GetTime, time_t(time_t* arg));
};

/**
 * Time is mocked to be one second after begining of an Epoch
 */
TEST(IgnoreNewExecutorsFilter, IgnoreAllExecutors) {
  DummySink<ResourceUsage> dummySink;
  PrinterSink<ResourceUsage> printer;
  MockIgnoreNewExecutorsFilter filter(200);
  JsonSource jsonSource;

  jsonSource.addConsumer(&filter);
  filter.addConsumer(&dummySink);

  EXPECT_CALL(filter, GetTime(nullptr)).Times(4).WillRepeatedly(Return(1));

  jsonSource.RunTests("tests/fixtures/baseline_smoke_test_resource_usage.json");

  ASSERT_EQ(dummySink.numberOfMessagesConsumed, 0);
}

/**
 * Time is mocked to be one second before end of an Epoch
 */
TEST(IgnoreNewExecutorsFilter, PassAllExecutors) {
  DummySink<ResourceUsage> dummySink;
  MockIgnoreNewExecutorsFilter filter(0);
  JsonSource jsonSource;

  jsonSource.addConsumer(&filter);
  filter.addConsumer(&dummySink);

  EXPECT_CALL(filter, GetTime(nullptr)).Times(4).WillRepeatedly(Return(2147483647));

  jsonSource.RunTests("tests/fixtures/baseline_smoke_test_resource_usage.json");

  ASSERT_EQ(dummySink.numberOfMessagesConsumed, 4);
}

/**
 * Time flows from 0 to 5
 * Filter argument is 0
 */
TEST(IgnoreNewExecutorsFilter, PassFiveExecutors) {
  DummySink<ResourceUsage> dummySink;
  PrinterSink<ResourceUsage> printer;
  MockIgnoreNewExecutorsFilter filter(0);
  JsonSource jsonSource;

  Sequence s1;
  for (int idx = 0; idx < IGNORE_NEW_EXECUTORS_SAMPLES; ++idx) {
    EXPECT_CALL(filter, GetTime(nullptr)).
        InSequence(s1).
        WillOnce(Return(idx));
  }

  jsonSource.addConsumer(&filter);
  filter.addConsumer(&dummySink);

  jsonSource.RunTests("tests/fixtures/ignore_new_executors.json");

  ASSERT_EQ(dummySink.numberOfMessagesConsumed, 5);
}

/**
 * Time flows from 0 to 5
 * Filter argument is 3
 */
TEST(IgnoreNewExecutorsFilter, PassTwoExecutors) {
  DummySink<ResourceUsage> dummySink;
  PrinterSink<ResourceUsage> printer;
  MockIgnoreNewExecutorsFilter filter(3);
  JsonSource jsonSource;

  Sequence s1;
  for (int idx = 0; idx < IGNORE_NEW_EXECUTORS_SAMPLES; ++idx) {
    EXPECT_CALL(filter, GetTime(nullptr)).
        InSequence(s1).
        WillOnce(Return(idx));
  }

  jsonSource.addConsumer(&filter);
  filter.addConsumer(&dummySink);

  jsonSource.RunTests("tests/fixtures/ignore_new_executors.json");

  ASSERT_EQ(dummySink.numberOfMessagesConsumed, 2);
}

}  // namespace tests {
}  // namespace serenity {
}  // namespace mesos {
