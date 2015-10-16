#include <list>
#include <string>

#include "detectors/rolling.hpp"

#include "gtest/gtest.h"

#include "stout/gtest.hpp"

#include "mesos/mesos.hpp"

#include "messages/serenity.hpp"

#include "filters/drop.hpp"

#include "serenity/data_utils.hpp"
#include "serenity/wid.hpp"

#include "tests/common/load_generator.hpp"
#include "tests/common/usage_helper.hpp"
#include "tests/common/sinks/mock_sink.hpp"
#include "tests/common/sources/mock_source.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using ::testing::DoAll;
using std::string;


/**
 * In this test we generate stable load and
 * test the RollingChangePointDetector. We don't expect
 * any contention.
 */
TEST(DropFilterRollingDetectorTest, StableLoad) {
  const uint64_t WINDOWS_SIZE = 10;
  const uint64_t CONTENTION_COOLDOWN = 10;
  const double_t RELATIVE_THRESHOLD = 0.5;
  const uint64_t LOAD_ITERATIONS = 100;
  // End of pipeline.
  MockSink<Contentions> mockSink;
  EXPECT_CALL(mockSink, consume(_))
    .Times(LOAD_ITERATIONS);

  DropFilter<RollingChangePointDetector> dropFilter(
      &mockSink, usage::getIpc,
      ChangePointDetectionState::createForRollingDetector(
          WINDOWS_SIZE, CONTENTION_COOLDOWN, RELATIVE_THRESHOLD));

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&dropFilter);

  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson("tests/fixtures/start_json_test.json");
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  LoadGenerator loadGen(
      [](double_t iter) { return 10; },
      new ZeroNoise(),
      LOAD_ITERATIONS);

  for (; loadGen.end(); loadGen++) {
    usage.mutable_executors(0)->CopyFrom(
        generateIPC(usage.executors(0),
                    (*loadGen)(),
                    (*loadGen).timestamp));

    // Run pipeline iteration.
    usageSource.produce(usage);

    if (loadGen.iteration > 0)
      mockSink.expectContentions(0);
  }
}


/**
 * In this test we generate stable load with drop and
 * test the RollingChangePointDetector. We expect one
 * contention.
 */
TEST(DropFilterRollingDetectorTest, StableLoadWithDrop) {
  const uint64_t WINDOWS_SIZE = 10;
  const uint64_t CONTENTION_COOLDOWN = 10;
  const double_t RELATIVE_THRESHOLD = 5;
  const uint64_t LOAD_ITERATIONS = 200;
  // End of pipeline.
  MockSink<Contentions> mockSink;
  EXPECT_CALL(mockSink, consume(_))
      .Times(LOAD_ITERATIONS);

  DropFilter<RollingChangePointDetector> dropFilter(
      &mockSink, usage::getIpc,
      ChangePointDetectionState::createForRollingDetector(
          WINDOWS_SIZE, CONTENTION_COOLDOWN, RELATIVE_THRESHOLD));

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&dropFilter);

  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson("tests/fixtures/start_json_test.json");
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  const double_t DROP_PROGRES = 1;
  LoadGenerator loadGen(
      [](double_t iter) { return 10; },
      new ZeroNoise(),
      LOAD_ITERATIONS);

  bool dropped = false;
  for (; loadGen.end(); loadGen++) {
    usage.mutable_executors(0)->CopyFrom(
        generateIPC(usage.executors(0),
                    (*loadGen)(),
                    (*loadGen).timestamp));

    // Run pipeline iteration.
    usageSource.produce(usage);

    if (dropped) {
      dropped = false;
      mockSink.expectContentionWithVictim("serenity2");
    } else {
      mockSink.expectContentions(0);
    }

    if (loadGen.iteration >= 100 &&
        loadGen.iteration < 110) {
      // After 6 iterations of 1 drop progress value should be below
      // threshold (4).
      if (loadGen.iteration == 105)
        dropped = true;
      loadGen.modifier -= DROP_PROGRES;
    }
  }
}

}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos
