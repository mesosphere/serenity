#include <list>

#include "gtest/gtest.h"

#include "stout/gtest.hpp"

#include "mesos/mesos.hpp"

#include "messages/serenity.hpp"

#include "filters/executor_age.hpp"

#include "observers/qos_correction.hpp"

#include "serenity/resource_helper.hpp"

#include "tests/common/usage_helper.hpp"
#include "tests/common/sinks/mock_sink.hpp"
#include "tests/common/sources/mock_source.hpp"

namespace mesos {
namespace serenity {
namespace tests {

// This fixture includes 5 executors:
// - 1 BE <1 CPUS> id 0
// - 2 BE <0.5 CPUS> id 1,2
// - 1 PR <4 CPUS> id 3
// - 1 PR <2 CPUS> id 4
const char QOS_FIXTURE[] = "tests/fixtures/qos/average_usage.json";

/**
 * Check if getRevocableExecutors function properly filters out PR executors.
 *
 * TODO(skonefal): Does it really work?
 */
TEST(HelperFunctionsTest, getRevocableExecutors) {
  Try<mesos::FixtureResourceUsage> usages = JsonUsage::ReadJson(QOS_FIXTURE);
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  std::list<ResourceUsage_Executor> ret =
    ResourceUsageHelper::getRevocableExecutors(usage);

  ASSERT_EQ(3u, ret.size());

  // Expected only BE executors.
  for (auto executor : ret) {
    Resources allocated(executor.allocated());
    EXPECT_FALSE(allocated.revocable().empty());
  }
}

}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos
