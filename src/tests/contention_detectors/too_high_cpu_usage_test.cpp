#include <list>
#include <string>

#include "contention_detectors/too_high_cpu.hpp"

#include "gtest/gtest.h"

#include "stout/gtest.hpp"

#include "mesos/mesos.hpp"

#include "messages/serenity.hpp"

#include "pwave/scenario.hpp"

#include "serenity/config.hpp"
#include "serenity/data_utils.hpp"
#include "serenity/wid.hpp"

#include "tests/common/config_helper.hpp"
#include "tests/common/signal_helper.hpp"
#include "tests/common/usage_helper.hpp"
#include "tests/common/sinks/mock_sink.hpp"
#include "tests/common/sources/mock_source.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using namespace pwave;  // NOLINT(build/namespaces)

using ::testing::DoAll;
using std::string;

/**
 * In this test we generate stable cpu utilization and
 * test the TooHighCpuUsageDetector.
 * We don't expect any contention.
 */
TEST(TooHighCpuUtilizationTest, LowUtilization) {
  const double_t UTIL_THRESHOLD = 0.72;
  const uint64_t ITERATIONS = 100;
  // End of pipeline.
  MockSink<Contentions> mockSink;
  EXPECT_CALL(mockSink, consume(_))
    .Times(ITERATIONS);

  TooHighCpuUsageDetector tooHighCpuUsageDetector(
    &mockSink, usage::getCpuUsage,
    createThresholdDetectorCfg(
      UTIL_THRESHOLD));

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&tooHighCpuUsageDetector);

  Try<mesos::FixtureResourceUsage> usages =
    JsonUsage::ReadJson("tests/fixtures/start_json_test.json");
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use([=](int64_t){ return 8*UTIL_THRESHOLD - 0.1; })
      .use(new ZeroNoise());


  ITERATE_SIGNAL(signalGen) {
    usage.mutable_executors(0)->CopyFrom(
      generateCpuUsage(usage.executors(0), (int64_t) (*signalGen)(), 1));

    // Run pipeline iteration.
    usageSource.produce(usage);

    if (signalGen.iteration > 0)
      mockSink.expectContentions(0);
  }
}


/**
 * In this test we generate stable cpu utilization and increase it after 20
 * iteration
 * We expect that TooHighCpuUsageDetector will trigger revoke all contention
 * after these 20 iterations.
 */
TEST(TooHighCpuUtilizationTest, HighUtilization) {
  const double_t UTIL_THRESHOLD = 0.72;
  const uint64_t ITERATIONS = 100;
  // End of pipeline.
  MockSink<Contentions> mockSink;
  EXPECT_CALL(mockSink, consume(_))
    .Times(ITERATIONS);

  TooHighCpuUsageDetector tooHighCpuUsageDetector(
    &mockSink, usage::getCpuUsage,
    createThresholdDetectorCfg(
      UTIL_THRESHOLD));

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&tooHighCpuUsageDetector);

  Try<mesos::FixtureResourceUsage> usages =
    JsonUsage::ReadJson("tests/fixtures/start_json_test.json");
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use([=](int64_t){ return 8*UTIL_THRESHOLD - 0.1; })
      .use(new ZeroNoise())
      .after(20).add(1);


  ITERATE_SIGNAL(signalGen) {
    usage.mutable_executors(0)->CopyFrom(
      generateCpuUsage(usage.executors(0), (int64_t) (*signalGen)(), 1));

    // Run pipeline iteration.
    usageSource.produce(usage);

    if (signalGen.iteration > 0)
      if (signalGen.iteration >= 20) mockSink.expectContentions(1);
      else mockSink.expectContentions(0);
  }
}


}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos
