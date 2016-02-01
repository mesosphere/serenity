#include <list>

#include "gtest/gtest.h"

#include "mesos/resources.hpp"

#include "mesos/slave/resource_estimator.hpp"

#include "mesos_modules/resource_estimator/serenity_estimator.hpp"

#include "stout/gtest.hpp"

#include "process/gtest.hpp"

#include "tests/common/usage_helper.hpp"

using std::list;

using mesos::slave::ResourceEstimator;

namespace mesos {
namespace serenity {
namespace tests {

class TestEstimationPipeline : public ResourceEstimatorPipeline {
 public:
  TestEstimationPipeline() {}

  virtual Result<Resources> run(const ResourceUsage& _product) {
    return Resources::parse("cpus(*):16");
  }
};


/**
 * This tests checks the interface.
 */
TEST(SerenityEstimatorTest, PipelineIntegration) {
  Try<ResourceEstimator*> resourceEstimator =
    serenity::SerenityEstimator::create(
      std::shared_ptr<ResourceEstimatorPipeline>(
          new TestEstimationPipeline()));
  ASSERT_SOME(resourceEstimator);

  ResourceEstimator* estimator = resourceEstimator.get();

  MockSlaveUsage usage(
      "tests/fixtures/baseline_smoke_test_resource_usage.json");

  Try<Nothing> initialize = estimator->initialize(
      lambda::bind(&MockSlaveUsage::usage, &usage));

  process::Future<Resources> result = estimator->oversubscribable();

  AWAIT_READY(result);

  for (Resources slack : result.get()) {
    for (Resource slack_resource : slack) {
      EXPECT_EQ("cpus", slack_resource.name());
      EXPECT_EQ(16, slack_resource.scalar().value());
    }
  }
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

