#include <list>
#include <string>

#include "filters/detectors/threshold.hpp"

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
 * Check if ThresholdFilter won't trigger any contention
 * under stable load below threshold.
 */
TEST(ThesholdDetectorTest, StableSignal) {
  const double_t UTILIZATION_THRESHOLD = 11;
  const uint64_t ITERATIONS = 30;

  ThresholdDetector assuranceDetector(
    Tag(QOS_CONTROLLER, "ThresholdDetector"),
    createThresholdDetectorCfg(UTILIZATION_THRESHOLD));

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


TEST(ThesholdDetectorTest, StableLoadBigIncrease) {
  const double_t UTILIZATION_THRESHOLD = 11;
  const uint64_t ITERATIONS = 30;

  ThresholdDetector assuranceDetector(
    Tag(QOS_CONTROLLER, "ThresholdDetector"),
    createThresholdDetectorCfg(UTILIZATION_THRESHOLD));

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use(math::const10Function)
      .use(new ZeroNoise())
      .after(10).add(5.0);  // Introduce sudden drop.

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


TEST(ThesholdDetectorTest, StableLoadProgressiveIncrease) {
  const double_t UTILIZATION_THRESHOLD = 11;
  const uint64_t ITERATIONS = 30;

  ThresholdDetector assuranceDetector(
    Tag(QOS_CONTROLLER, "ThresholdDetector"),
    createThresholdDetectorCfg(UTILIZATION_THRESHOLD));

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use(math::const10Function)
      .use(new ZeroNoise())
      .after(10).constantAdd(0.2, 10);  // Introduce sudden growth.

  ITERATE_SIGNAL(signalGen) {
    Result<Detection> result =
      assuranceDetector.processSample((*signalGen)());

    if (signalGen.iteration >= 15) {
      EXPECT_SOME(result);
    } else {
      EXPECT_NONE(result);
    }
  }
}

}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos
