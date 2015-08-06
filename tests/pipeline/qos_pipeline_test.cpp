#include <list>

#include "gtest/gtest.h"

#include "filters/drop.hpp"

#include "process/clock.hpp"
#include "process/gtest.hpp"

#include "messages/serenity.hpp"

#include "pipeline/qos_pipeline.hpp"

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
        ChangePointDetectionState::createForRollingDetector(
            WINDOWS_SIZE,
            CONTENTION_COOLDOWN,
            RELATIVE_THRESHOLD), false, true);

  Result<QoSCorrections> corrections = pipeline->run(usage);
  EXPECT_NONE(corrections);

  delete pipeline;
}


TEST(QoSPipelineTest, NoCorrections) {
  uint64_t WINDOWS_SIZE = 10;
  uint64_t CONTENTION_COOLDOWN = 10;
  double_t RELATIVE_THRESHOLD = 0.5;

  MockSlaveUsage mockSlaveUsage(
      "tests/fixtures/pipeline/sufficient_metrics.json");

  QoSControllerPipeline* pipeline =
      new CpuQoSPipeline<RollingChangePointDetector>(
          ChangePointDetectionState::createForRollingDetector(
              WINDOWS_SIZE,
              CONTENTION_COOLDOWN,
              RELATIVE_THRESHOLD), false, true);

  Result<QoSCorrections> corrections =
      pipeline->run(mockSlaveUsage.usage().get());
  EXPECT_NONE(corrections);

  delete pipeline;
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

