#include <gtest/gtest.h>

#include <stout/gtest.hpp>

#include <mesos/resources.hpp>

#include "filters/utilization_threshold.hpp"

#include "process/future.hpp"

#include "tests/common/sinks/mock_sink.hpp"
#include "tests/common/sources/json_source.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using ::testing::DoAll;


TEST(UtilizationThresholdFilterTest, OkLoad) {
  // End of pipeline.
  MockSink<ResourceUsage> mockSink;
  process::Future<ResourceUsage> usage;
  EXPECT_CALL(mockSink, consume(_))
    .WillOnce(InvokeConsumeUsageCountExecutors(&mockSink, 1));

  // Second component in pipeline.
  UtilizationThresholdFilter utilizationFilter(&mockSink);

  // First component in pipeline.
  JsonSource jsonSource(&utilizationFilter);

  // Start test.
  // This fixture includes three pipeline loops:
  // - ResourceUsage with cpu statistics included.
  //    First loop so allocated resources will be used.
  //    allocated ~== total so oversubscription will be blocked.
  // - ResourceUsage with cpu statistics included.
  //    cpu_usage < total so oversubscription will be passed.
  // - ResourceUsage without cpu statistics included.
  //    In that case allocated resources will be used.
  //    allocated ~== total so oversubscription will be blocked.
  ASSERT_SOME(jsonSource.RunTests(
      "tests/fixtures/utilization_threshold/ok_load_test.json"));
}


TEST(UtilizationThresholdFilterTest, TooHighLoad) {
  // End of pipeline.
  MockSink<ResourceUsage> mockSink;
  EXPECT_CALL(mockSink, consume(_))
    .Times(0);

  // Second component in pipeline.
  UtilizationThresholdFilter utilizationFilter(&mockSink);

  // First component in pipeline.
  JsonSource jsonSource(&utilizationFilter);

  // Start test.
  // This fixture includes two pipeline loops:
  // - ResourceUsage with cpu statistics included.
  //    First loop so allocated resources will be used.
  //    allocated ~== total so oversubscription will be blocked.
  // - ResourceUsage with cpu statistics included.
  //    cpu_usage ~== total so oversubscription will be blocked.
  ASSERT_SOME(jsonSource.RunTests(
      "tests/fixtures/utilization_threshold/too_high_load_test.json"));
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

