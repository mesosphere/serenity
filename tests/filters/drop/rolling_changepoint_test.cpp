#include <list>
#include <string>

#include "gtest/gtest.h"

#include "stout/gtest.hpp"

#include "filters/drop.hpp"

#include "serenity/data_utils.hpp"

#include "tests/common/load_generator.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using std::string;

/**
 * Check if RollingChangePointDetector won't detect any change point
 * in stable load.
 */
TEST(RollingChangePointDetectionTest, StableLoadNoChangePoint) {
  const uint64_t WINDOWS_SIZE = 10;
  const uint64_t CONTENTION_COOLDOWN = 10;
  const double_t RELATIVE_THRESHOLD = 10;
  const uint64_t LOAD_ITERATIONS = 100;
  RollingChangePointDetector rollingChangePointDetector;
  rollingChangePointDetector.configure(
      ChangePointDetectionState::createForRollingDetector(
          WINDOWS_SIZE, CONTENTION_COOLDOWN, RELATIVE_THRESHOLD));

  LoadGenerator loadGen(
      [](double_t iter) { return 10; }, new ZeroNoise(), LOAD_ITERATIONS);

  for (; loadGen.end() ; loadGen++) {
    Result<ChangePointDetection> result =
        rollingChangePointDetector.processSample((*loadGen)());

    EXPECT_NONE(result);
  }
}

/**
 * Check if RollingChangePointDetector will detect two contentions
 * in stable load with drop.
 */
TEST(RollingChangePointDetectionTest, StableLoadOneChangePoint) {
  const uint64_t WINDOWS_SIZE = 10;
  const uint64_t CONTENTION_COOLDOWN = 10;
  const double_t RELATIVE_THRESHOLD = 10;
  const double_t DROP_PROGRES = 2;
  const uint64_t LOAD_ITERATIONS = 200;
  RollingChangePointDetector rollingChangePointDetector;
  rollingChangePointDetector.configure(
      ChangePointDetectionState::createForRollingDetector(
          WINDOWS_SIZE, CONTENTION_COOLDOWN, RELATIVE_THRESHOLD));

  LoadGenerator loadGen(
      [](double_t iter) { return 10; }, new ZeroNoise(), LOAD_ITERATIONS);

  // Starting iterations.
  for (; loadGen.end() ; loadGen++) {
    // Running detector.
    Result<ChangePointDetection> result =
      rollingChangePointDetector.processSample((*loadGen)());

    // In 106th iteration we expect to detect one contention.
    if (loadGen.iteration == 106)
      EXPECT_SOME(result);
    else
      EXPECT_NONE(result);

    // Introduce a drop in the middle of the test..
    if (loadGen.iteration >= 100 &&
        loadGen.iteration < 110) {
      // After 6 iterations of 2 drop progress value should be below
      // threshold (-2).
      loadGen.modifier -= DROP_PROGRES;
    }
  }
}

}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos
