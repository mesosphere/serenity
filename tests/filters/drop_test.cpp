#include <list>
#include <string>

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

/**
 * Check if NaiveChangePointDetector won't detect any change point
 * in stable load.
 */
TEST(MeanChangePointDetectionTest, StableLoadNoChangePoint) {
  const double_t THRESHOLD = 0;
  const int32_t LOAD_ITERATIONS = 100;
  NaiveChangePointDetector naiveChangePointDetector(10, THRESHOLD);

  LoadGenerator loadGen(
      math::constFunction, new ZeroNoise(), LOAD_ITERATIONS);

  for (; loadGen.end() ; loadGen++) {
    Result<ChangePointDetection> result =
      naiveChangePointDetector.processSample((*loadGen)());

    EXPECT_NONE(result);
  }
}


/**
 * Check if NaiveChangePointDetector won't detect any change point
 * in noisy load.
 */
TEST(MeanChangePointDetectionTest, NoisyLoadNoChangePoint) {
  const double_t THRESHOLD = 0;
  const double_t MAX_NOISE = 9;
  const int32_t LOAD_ITERATIONS = 100;
  NaiveChangePointDetector naiveChangePointDetector(10, THRESHOLD);

  LoadGenerator loadGen(
      math::constFunction,
      new SymetricNoiseGenerator(MAX_NOISE),
      LOAD_ITERATIONS);

  for (; loadGen.end() ; loadGen++) {
    Result<ChangePointDetection> result =
        naiveChangePointDetector.processSample((*loadGen)());

    EXPECT_NONE(result);
  }
}


/**
 * Check if NaiveChangePointDetector will detect one change point
 * in stable load.
 */
TEST(MeanChangePointDetectionTest, StableLoadOneChangePoint) {
  const double_t THRESHOLD = 0;
  const int32_t LOAD_ITERATIONS = 200;
  const double_t DROP_PROGRES = 1;
  NaiveChangePointDetector naiveChangePointDetector(10, THRESHOLD);

  LoadGenerator loadGen(
      math::constFunction, new ZeroNoise(), LOAD_ITERATIONS);
  bool dropped = false;
  for (; loadGen.end() ; loadGen++) {
    // Introduce a drop in the middle of the test..
    if (loadGen.iteration > 100 &&
        loadGen.iteration < 150) {
      // After 11 iterations of 1 drop progress value should be below
      // threshold (-1).
      if (loadGen.iteration > 111 && !dropped) dropped = true;
      loadGen.modifier -= DROP_PROGRES;
    }
    Result<ChangePointDetection> result =
        naiveChangePointDetector.processSample((*loadGen)());

    if (dropped) EXPECT_SOME(result);
    else
      EXPECT_NONE(result);
  }
}


// TODO(bplotka): Do better tests.
TEST(DropFilterTest, StableLoad) {
  // End of pipeline.
  MockSink<Contentions> mockSink;
  process::Future<Contentions> contentions;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&contentions),
          Return(Nothing())));

  DropFilter<NaiveChangePointDetector> dropFilter(
      &mockSink, usage::getIpc, 10, 0.1);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&dropFilter);

  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson(QOS_FIXTURE);
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  usageSource.produce(usage);

  EXPECT_TRUE(contentions.isReady());

  EXPECT_TRUE(contentions.get().empty());
}

}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos
