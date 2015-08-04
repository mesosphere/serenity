#include <list>

#include "gtest/gtest.h"

#include "mesos/resources.hpp"

#include "process/clock.hpp"
#include "process/gtest.hpp"

#include "pipeline/estimator_pipeline.hpp"

#include "stout/gtest.hpp"

#include "tests/common/usage_helper.hpp"

using std::list;

namespace mesos {
namespace serenity {
namespace tests {

TEST(EstimatorPipelineTest, FiltersNotProperlyFed) {
  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson("tests/fixtures/pipeline/insufficient_metrics.json");
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  ResourceEstimatorPipeline* pipeline = new CpuEstimatorPipeline();

  Result<Resources> slack = pipeline->run(usage);
  EXPECT_NONE(slack);

  delete pipeline;
}


TEST(EstimatorPipelineTest, NoSlack) {
  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson("tests/fixtures/pipeline/sufficient_metrics.json");
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  ResourceEstimatorPipeline* pipeline = new CpuEstimatorPipeline();

  Result<Resources> slack = pipeline->run(usage);
  ASSERT_SOME(slack);

  EXPECT_TRUE(slack.get().empty());

  delete pipeline;
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

