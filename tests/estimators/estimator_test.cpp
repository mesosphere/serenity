#include <gtest/gtest.h>

#include <list>

#include <mesos/resources.hpp>

#include <mesos/slave/resource_estimator.hpp>

#include <stout/gtest.hpp>

#include <process/gtest.hpp>

#include "estimator/serenity_estimator.hpp"

#include "tests/common/serenity.hpp"

using std::list;

using mesos::slave::ResourceEstimator;

namespace mesos {
namespace serenity {
namespace tests {

// NOTE: For now checking only the interface.
TEST(SerenityEstimatorTest, EmptySlackEstimation)
{
  Try<ResourceEstimator*> resourceEstimator =
    serenity::SerenityEstimator::create(None());
  ASSERT_SOME(resourceEstimator);

  ResourceEstimator* estimator = resourceEstimator.get();

  MockSlaveUsage usage(5);

  Try<Nothing> initialize = estimator->initialize(
      lambda::bind(&MockSlaveUsage::usage, &usage));

  process::Future<Resources> result = estimator->oversubscribable();

  AWAIT_READY(result);

  for(Resources slack : result.get()){
    for(Resource slack_resource : slack) {
      EXPECT_TRUE(Resources::isEmpty(slack_resource));
    }
  }

}

} // namespace tests {
} // namespace serenity {
} // namespace mesos {

