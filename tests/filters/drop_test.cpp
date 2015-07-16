#include <list>
#include <string>

#include "gtest/gtest.h"

#include "stout/gtest.hpp"

#include "mesos/mesos.hpp"

#include "messages/serenity.hpp"

#include "observers/qos_correction.hpp"

#include "serenity/wid.hpp"

#include "tests/common/usage_helper.hpp"
#include "tests/common/sinks/mock_sink.hpp"
#include "tests/common/sources/mock_source.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using ::testing::DoAll;
using std::string;

// This fixture includes 5 executors:
// - 1 BE <1 CPUS> id 0
// - 2 BE <0.5 CPUS> id 1,2
// - 1 PR <4 CPUS> id 3
// - 1 PR <2 CPUS> id 4
const char QOS_FIXTURE[] = "tests/fixtures/qos/average_usage.json";
const int BE_1CPUS = 0;
const int BE_0_5CPUS_1 = 1;
const int BE_0_5CPUS_2 = 2;
const int PR_4CPUS = 3;
const int PR_2CPUS = 4;


TEST(DropFilterTest, StableLoad) {
  // End of pipeline.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&corrections),
          Return(Nothing())));

  QoSCorrectionObserver observer(
      &mockSink, 1, ContentionInterpreters::severityBasedCpuContention);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&observer);

  // ..DropFilter as a source of Contention.

  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson(QOS_FIXTURE);
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  usageSource.produce(usage);

  EXPECT_TRUE(corrections.isReady());

  EXPECT_TRUE(corrections.get().empty());
}

}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos
