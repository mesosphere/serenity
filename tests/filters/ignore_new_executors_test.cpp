#include "filters/ignore_new_executors.hpp"

#include "gmock/gmock.h"

#include "tests/common/serenity.hpp"
#include "tests/common/sinks/dummy_sink.hpp"
#include "tests/common/sources/json_source.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using testing::Return;
using testing::Sequence;

constexpr int IGNORE_NEW_EXECUTORS_SAMPLES = 6;


class MockIgnoreNewExecutorsFilter : public IgnoreNewExecutorsFilter {
 public:
  MockIgnoreNewExecutorsFilter(
      Consumer<ResourceUsage>* _consumer,
      uint32_t _threshold = 5 * 60) :
        IgnoreNewExecutorsFilter(_consumer, _threshold) {}

  MOCK_METHOD1(GetTime, time_t(time_t* arg));
};


/**
 * Time is mocked to be one second after begining of an Epoch
 */
TEST(IgnoreNewExecutorsFilter, IgnoreAllExecutors) {
  DummySink<ResourceUsage> dummySink;
  MockIgnoreNewExecutorsFilter filter(&dummySink, 200);
  JsonSource jsonSource(&filter);

  EXPECT_CALL(filter, GetTime(nullptr))
      .Times(4)
      .WillRepeatedly(Return(1));

  jsonSource.RunTests("tests/fixtures/"
                      "baseline_smoke_test_resource_usage.json");

  ASSERT_EQ(dummySink.numberOfMessagesConsumed, 0);
}


/**
 * Time is mocked to be one second before end of an Epoch
 */
TEST(IgnoreNewExecutorsFilter, PassAllExecutors) {
  DummySink<ResourceUsage> dummySink;
  MockIgnoreNewExecutorsFilter filter(&dummySink, 0);
  JsonSource jsonSource(&filter);

  EXPECT_CALL(filter, GetTime(nullptr))
      .Times(4)
      .WillRepeatedly(Return(2147483647));

  jsonSource.RunTests("tests/fixtures/"
                      "baseline_smoke_test_resource_usage.json");

  ASSERT_EQ(dummySink.numberOfMessagesConsumed, 4);
}


/**
 * Time flows from 0 to 5
 * Filter argument is 0
 */
TEST(IgnoreNewExecutorsFilter, PassFiveExecutors) {
  DummySink<ResourceUsage> dummySink;
  MockIgnoreNewExecutorsFilter filter(&dummySink, 0);
  JsonSource jsonSource(&filter);

  Sequence s1;
  for (int idx = 0; idx < IGNORE_NEW_EXECUTORS_SAMPLES; ++idx) {
    EXPECT_CALL(filter, GetTime(nullptr))
        .InSequence(s1)
        .WillOnce(Return(idx));
  }

  jsonSource.RunTests("tests/fixtures/ignore_new_executors.json");

  ASSERT_EQ(dummySink.numberOfMessagesConsumed, 5);
}


/**
 * Time flows from 0 to 5
 * Filter argument is 3
 */
TEST(IgnoreNewExecutorsFilter, PassTwoExecutors) {
  DummySink<ResourceUsage> dummySink;
  MockIgnoreNewExecutorsFilter filter(&dummySink, 3);
  JsonSource jsonSource(&filter);

  Sequence s1;
  for (int idx = 0; idx < IGNORE_NEW_EXECUTORS_SAMPLES; ++idx) {
    EXPECT_CALL(filter, GetTime(nullptr))
        .InSequence(s1)
        .WillOnce(Return(idx));
  }

  jsonSource.RunTests("tests/fixtures/ignore_new_executors.json");

  ASSERT_EQ(dummySink.numberOfMessagesConsumed, 2);
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos
