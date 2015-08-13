#include <list>

#include "gtest/gtest.h"

#include "filters/drop.hpp"

#include "process/clock.hpp"
#include "process/gtest.hpp"

#include "messages/serenity.hpp"

#include "pipeline/qos_pipeline.hpp"

#include "serenity/config.hpp"

#include "stout/gtest.hpp"

#include "tests/common/usage_helper.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(QoSPipelineTest, FiltersNotProperlyFed) {
  uint64_t WINDOWS_SIZE = 10;
  uint64_t CONTENTION_COOLDOWN = 10;
  double_t RELATIVE_THRESHOLD = 0.5;

  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson("tests/fixtures/pipeline/insufficient_metrics.json");
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  QoSControllerPipeline* pipeline =
      new CpuQoSPipeline<RollingChangePointDetector>(
          QoSPipelineConf(
              ChangePointDetectionState::createForRollingDetector(
                  WINDOWS_SIZE,
                  CONTENTION_COOLDOWN,
                  RELATIVE_THRESHOLD),
              ema::DEFAULT_ALPHA,
              utilization::DEFAULT_THRESHOLD,
              false,
              true));

  Result<QoSCorrections> corrections = pipeline->run(usage);
  EXPECT_NONE(corrections);

  delete pipeline;
}


// This fixture includes 5 executors:
// - 1 BE <1 CPUS> id 0
// - 2 BE <0.5 CPUS> id 1,2
// - 1 PR <4 CPUS> id 3
// - 1 PR <2 CPUS> id 4
// Iterations:
// 0) 3, 4 has IPC = 1
// 1) 3, 4 has IPC = 1 for stable reference.
const char QOS_PIPELINE_FIXTURE1[] =
    "tests/fixtures/pipeline/qos_no_correction.json";
const int BE_1CPUS = 0;
const int BE_0_5CPUS_1 = 1;
const int BE_0_5CPUS_2 = 2;
const int PR_4CPUS = 3;
const int PR_2CPUS = 4;

TEST(QoSPipelineTest, NoCorrections) {
  uint64_t WINDOWS_SIZE = 10;
  uint64_t CONTENTION_COOLDOWN = 10;
  double_t RELATIVE_THRESHOLD = 0.5;

  MockSlaveUsage mockSlaveUsage(QOS_PIPELINE_FIXTURE1);

  QoSControllerPipeline* pipeline =
      new CpuQoSPipeline<RollingChangePointDetector>(
          QoSPipelineConf(
              ChangePointDetectionState::createForRollingDetector(
                  WINDOWS_SIZE,
                  CONTENTION_COOLDOWN,
                  RELATIVE_THRESHOLD),
              ema::DEFAULT_ALPHA,
              utilization::DEFAULT_THRESHOLD,
              false,
              true));

  // First iteration.
  Result<QoSCorrections> corrections =
      pipeline->run(mockSlaveUsage.usage().get());
  EXPECT_NONE(corrections);

  // Second iteration.
  corrections = pipeline->run(mockSlaveUsage.usage().get());
  EXPECT_SOME(corrections);

  EXPECT_TRUE(corrections.get().empty());

  delete pipeline;
}


// This fixture includes 5 executors:
// - 1 BE <1 CPUS> id 0
// - 2 BE <0.5 CPUS> id 1,2
// - 1 PR <4 CPUS> id 3
// - 1 PR <2 CPUS> id 4
// Iterations:
// 0) 3, 4 have IPC = 1
// 1) 3, 4 have IPC = 1 for stable reference.
// 2) 3 has IPC 0.5, 4 has IPC 1
const char QOS_PIPELINE_FIXTURE2[] =
    "tests/fixtures/pipeline/qos_one_drop_correction.json";

TEST(QoSPipelineTest, RollingDetectorOneDropCorrections) {
  uint64_t WINDOWS_SIZE = 10;
  uint64_t CONTENTION_COOLDOWN = 10;
  double_t RELATIVE_THRESHOLD = 0.4;

  MockSlaveUsage mockSlaveUsage(QOS_PIPELINE_FIXTURE2);

  QoSControllerPipeline* pipeline =
      new CpuQoSPipeline<RollingChangePointDetector>(
          QoSPipelineConf(
              ChangePointDetectionState::createForRollingDetector(
                  WINDOWS_SIZE,
                  CONTENTION_COOLDOWN,
                  RELATIVE_THRESHOLD),
              ema::DEFAULT_ALPHA,
              utilization::DEFAULT_THRESHOLD,
              false,
              true));

  // First iteration.
  Result<QoSCorrections> corrections =
      pipeline->run(mockSlaveUsage.usage().get());
  EXPECT_NONE(corrections);

  // Second iteration.
  corrections = pipeline->run(mockSlaveUsage.usage().get());
  EXPECT_SOME(corrections);
  EXPECT_TRUE(corrections.get().empty());

  // TODO!
  for (int i = 0; i < 2*WINDOWS_SIZE; i++) {
    // Third iteration (repeated).
    corrections = pipeline->run(mockSlaveUsage.usageIter(2).get());
    EXPECT_SOME(corrections);
    EXPECT_TRUE(corrections.get().empty());
  }

  delete pipeline;
}


}  // namespace tests
}  // namespace serenity
}  // namespace mesos

