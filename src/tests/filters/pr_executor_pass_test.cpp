#include <gtest/gtest.h>

#include <stout/gtest.hpp>

#include <mesos/resources.hpp>

#include "filters/pr_executor_pass.hpp"

#include "process/future.hpp"

#include "tests/common/sinks/mock_sink.hpp"
#include "tests/common/sources/json_source.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using ::testing::DoAll;

TEST(PrTasksFilterTest, BeTasksFilteredOut) {
  // End of pipeline.
  MockSink<ResourceUsage> mockSink;
  process::Future<ResourceUsage> usage;
  EXPECT_CALL(mockSink, consume(_))
    .WillOnce(DoAll(
       FutureArg<0>(&usage),
       Return(Nothing())));

  // Second component in pipeline.
  PrExecutorPassFilter prTasksFilter(&mockSink);

  // First component in pipeline.
  JsonSource jsonSource(&prTasksFilter);

  // Start test.
  ASSERT_SOME(jsonSource.RunTests("tests/fixtures/pr_executor_pass/test.json"));

  ASSERT_TRUE(usage.isReady());
  ASSERT_EQ(1u, usage.get().executors().size());

  Resources allocated(usage.get().executors(0).allocated());
  EXPECT_TRUE(allocated.revocable().empty());
}


TEST(PrTasksFilterTest, EmptyAllocatedResources) {
  // End of pipeline.
  MockSink<ResourceUsage> mockSink;
  EXPECT_CALL(mockSink, consume(_))
      .Times(0);

  // Second component in pipeline.
  PrExecutorPassFilter prTasksFilter(&mockSink);

  // First component in pipeline.
  JsonSource jsonSource(&prTasksFilter);

  // Start test.
  ASSERT_SOME(jsonSource.RunTests(
      "tests/fixtures/pr_executor_pass/insufficient_metrics_test.json"));

  EXPECT_EQ(0, mockSink.numberOfMessagesConsumed);
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

