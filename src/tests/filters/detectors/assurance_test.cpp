#include <list>
#include <string>

#include "filters/detectors/assurance.hpp"

#include "gtest/gtest.h"

#include "pwave/scenario.hpp"

#include "stout/gtest.hpp"

#include "serenity/data_utils.hpp"

#include "tests/common/config_helper.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using namespace pwave;  // NOLINT(build/namespaces)

using std::string;

/**
 * Check if AssuranceDetector won't detect any change point
 * under stable load.
 */
TEST(AssuranceDetectorTest, StableSignal) {
  const uint64_t WINDOWS_SIZE = 10;
  const uint64_t CHECKPOINTS = 3;
  const double_t FRACTION_THRESHOLD = 0.5;
  const double_t SEVERITY_FRACTION = 0;
  const double_t NEAR_FRACTION = 0;
  const uint64_t ITERATIONS = 100;

  AssuranceDetector assuranceDetector(
    Tag(QOS_CONTROLLER, "AssuranceDetector"),
    createAssuranceDetectorCfg(
        WINDOWS_SIZE,
        CHECKPOINTS,
        FRACTION_THRESHOLD,
        SEVERITY_FRACTION,
        NEAR_FRACTION));

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use(math::const10Function)
      .use(new ZeroNoise());

  ITERATE_SIGNAL(signalGen) {
    Result<Detection> result =
      assuranceDetector.processSample((*signalGen)());

    EXPECT_NONE(result);
  }
}


TEST(AssuranceDetectorTest, StableLoadOneBigDrop) {
  const uint64_t WINDOWS_SIZE = 10;
  const uint64_t CHECKPOINTS = 3;
  const double_t FRACTION_THRESHOLD = 0.5;
  const double_t SEVERITY_FRACTION = 1;
  const double_t NEAR_FRACTION = 0.1;
  const uint64_t QUORUM = 2;
  const uint64_t ITERATIONS = 100;

  AssuranceDetector assuranceDetector(
    Tag(QOS_CONTROLLER, "AssuranceDetector"),
    createAssuranceDetectorCfg(
      WINDOWS_SIZE,
      CHECKPOINTS,
      FRACTION_THRESHOLD,
      SEVERITY_FRACTION,
      NEAR_FRACTION,
      QUORUM));

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use(math::const10Function)
      .use(new ZeroNoise())
      .after(10).add(-5.0);

  ITERATE_SIGNAL(signalGen) {
    Result<Detection> result =
      assuranceDetector.processSample((*signalGen)());

    if (signalGen.iteration >= 10) {
      EXPECT_SOME(result);
    } else {
      EXPECT_NONE(result);
    }
  }
}


TEST(AssuranceDetectorTest, StableLoadOneBigDropWithReset) {
  const uint64_t WINDOWS_SIZE = 10;
  const uint64_t CHECKPOINTS = 3;
  const double_t FRACTION_THRESHOLD = 0.5;
  const double_t SEVERITY_FRACTION = 1;
  const double_t NEAR_FRACTION = 0.1;
  const uint64_t QUORUM = 2;
  const uint64_t ITERATIONS = 16;

  AssuranceDetector assuranceDetector(
    Tag(QOS_CONTROLLER, "AssuranceDetector"),
    createAssuranceDetectorCfg(
      WINDOWS_SIZE,
      CHECKPOINTS,
      FRACTION_THRESHOLD,
      SEVERITY_FRACTION,
      NEAR_FRACTION,
      QUORUM));

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use(math::const10Function)
      .use(new ZeroNoise())
      .after(10).add(-5.0);

  ITERATE_SIGNAL(signalGen) {
    Result<Detection> result =
      assuranceDetector.processSample((*signalGen)());

    if (signalGen.iteration >= 10 && signalGen.iteration < 20) {
      EXPECT_SOME(result);
      assuranceDetector.reset();
    } else {
      EXPECT_NONE(result);
    }
  }
}


}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos
