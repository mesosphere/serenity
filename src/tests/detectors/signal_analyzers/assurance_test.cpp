#include <list>
#include <string>

#include "detectors/signal_analyzers/assurance.hpp"

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
 * Check if AssuranceDropAnalyzer won't detect any change point
 * under stable load.
 */
TEST(AssuranceDropAnalyzerTest, StableSignal) {
  const uint64_t WINDOWS_SIZE = 8;
  const uint64_t MAX_CHECKPOINTS = 4;
  const double_t FRACTION_THRESHOLD = 0.5;
  const double_t SEVERITY_FRACTION = 0;
  const double_t NEAR_FRACTION = 0;
  const uint64_t ITERATIONS = 30;

  AssuranceDropAnalyzer AssuranceDropAnalyzer(
    Tag(QOS_CONTROLLER, "AssuranceDropAnalyzer"),
    createAssuranceAnalyzerCfg(
        WINDOWS_SIZE,
        MAX_CHECKPOINTS,
        FRACTION_THRESHOLD,
        SEVERITY_FRACTION,
        NEAR_FRACTION));

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use(math::const10Function)
      .use(new ZeroNoise());

  ITERATE_SIGNAL(signalGen) {
    Result<Detection> result =
      AssuranceDropAnalyzer.processSample((*signalGen)());

    EXPECT_NONE(result);
  }
}


TEST(AssuranceDropAnalyzerTest, StableLoadOneBigDrop) {
  const uint64_t WINDOWS_SIZE = 8;
  const uint64_t MAX_CHECKPOINTS = 4;
  const double_t FRACTION_THRESHOLD = 0.5;
  const double_t SEVERITY_FRACTION = 1;
  const double_t NEAR_FRACTION = 0.1;
  const double_t QUORUM = 0.5;
  const uint64_t ITERATIONS = 30;

  AssuranceDropAnalyzer AssuranceDropAnalyzer(
    Tag(QOS_CONTROLLER, "AssuranceDropAnalyzer"),
    createAssuranceAnalyzerCfg(
      WINDOWS_SIZE,
      MAX_CHECKPOINTS,
      FRACTION_THRESHOLD,
      SEVERITY_FRACTION,
      NEAR_FRACTION,
      QUORUM));

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use(math::const10Function)
      .use(new ZeroNoise())
      .after(10).add(-5.0);  // Introduce sudden drop.

  ITERATE_SIGNAL(signalGen) {
    Result<Detection> result =
      AssuranceDropAnalyzer.processSample((*signalGen)());

    if (signalGen.iteration >= 10) {
      EXPECT_SOME(result);
    } else {
      EXPECT_NONE(result);
    }
  }
}


TEST(AssuranceDropAnalyzerTest, StableLoadOneBigDropWithReset) {
  const uint64_t WINDOWS_SIZE = 8;
  const uint64_t MAX_CHECKPOINTS = 4;
  const double_t FRACTION_THRESHOLD = 0.5;
  const double_t SEVERITY_FRACTION = 1;
  const double_t NEAR_FRACTION = 0.1;
  const double_t QUORUM = 0.50;
  const uint64_t ITERATIONS = 30;

  AssuranceDropAnalyzer AssuranceDropAnalyzer(
    Tag(QOS_CONTROLLER, "AssuranceDropAnalyzer"),
    createAssuranceAnalyzerCfg(
      WINDOWS_SIZE,
      MAX_CHECKPOINTS,
      FRACTION_THRESHOLD,
      SEVERITY_FRACTION,
      NEAR_FRACTION,
      QUORUM));

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use(math::const10Function)
      .use(new ZeroNoise())
      .after(10).add(-5.0);  // Introduce sudden drop.

  ITERATE_SIGNAL(signalGen) {
    Result<Detection> result =
      AssuranceDropAnalyzer.processSample((*signalGen)());

    // Detector should stop detecting after 4 iterations, since there are
    // 4 checkpoints with quorum 3 (so we look in the past T-4 iterations).
    if (signalGen.iteration >= 10 && signalGen.iteration < (10+4)) {
      EXPECT_SOME(result);
      AssuranceDropAnalyzer.reset();
    } else {
      EXPECT_NONE(result);
    }
  }
}


TEST(AssuranceDropAnalyzerTest, StableLoadOneProgressiveDrop) {
  const uint64_t WINDOWS_SIZE = 8;
  const uint64_t MAX_CHECKPOINTS = 4;
  const double_t FRACTION_THRESHOLD = 0.5;
  const double_t SEVERITY_FRACTION = 1;
  const double_t NEAR_FRACTION = 0.1;
  const double_t QUORUM = 0.5;
  const uint64_t ITERATIONS = 30;

  AssuranceDropAnalyzer AssuranceDropAnalyzer(
    Tag(QOS_CONTROLLER, "AssuranceDropAnalyzer"),
    createAssuranceAnalyzerCfg(
      WINDOWS_SIZE,
      MAX_CHECKPOINTS,
      FRACTION_THRESHOLD,
      SEVERITY_FRACTION,
      NEAR_FRACTION,
      QUORUM));

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use(math::const10Function)
      .use(new ZeroNoise())
      .after(10).constantAdd(-1, 10);  // Introduce constant drop.

  ITERATE_SIGNAL(signalGen) {
    Result<Detection> result =
      AssuranceDropAnalyzer.processSample((*signalGen)());

    if (signalGen.iteration >= 15) {
      EXPECT_SOME(result);
    } else {
      EXPECT_NONE(result);
    }
  }
}


TEST(AssuranceDropAnalyzerTest, StableLoadOneBigDropAndRecovery) {
  const uint64_t WINDOWS_SIZE = 8;
  const uint64_t MAX_CHECKPOINTS = 4;
  const double_t FRACTION_THRESHOLD = 0.5;
  const double_t SEVERITY_FRACTION = 1;
  const double_t NEAR_FRACTION = 0.1;
  const double_t QUORUM = 0.5;
  const uint64_t ITERATIONS = 30;

  AssuranceDropAnalyzer AssuranceDropAnalyzer(
    Tag(QOS_CONTROLLER, "AssuranceDropAnalyzer"),
    createAssuranceAnalyzerCfg(
      WINDOWS_SIZE,
      MAX_CHECKPOINTS,
      FRACTION_THRESHOLD,
      SEVERITY_FRACTION,
      NEAR_FRACTION,
      QUORUM));

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use(math::const10Function)
      .use(new ZeroNoise())
      .after(10).add(-5.0)  // Introduce sudden drop.
      .after(5).constantAdd(1.0, 4);  // Introduce constant increase.

  ITERATE_SIGNAL(signalGen) {
    Result<Detection> result =
      AssuranceDropAnalyzer.processSample((*signalGen)());

    if (signalGen.iteration >= 10 && signalGen.iteration < 18) {
      EXPECT_SOME(result);
    } else {
      EXPECT_NONE(result);
    }
  }
}


TEST(AssuranceDropAnalyzerTest, NoisyLoadOneBigDropLessCheckpoints) {
  const uint64_t WINDOWS_SIZE = 8;
  const double_t QUORUM = 0.70;
  const uint64_t MAX_CHECKPOINTS = 4;

  const double_t FRACTION_THRESHOLD = 0.5;
  const double_t SEVERITY_FRACTION = 1;
  const double_t NEAR_FRACTION = 0.1;
  const uint64_t ITERATIONS = 30;
  const uint64_t MAX_NOISE = 4;

  AssuranceDropAnalyzer AssuranceDropAnalyzer(
    Tag(QOS_CONTROLLER, "AssuranceDropAnalyzer"),
    createAssuranceAnalyzerCfg(
      WINDOWS_SIZE,
      MAX_CHECKPOINTS,
      FRACTION_THRESHOLD,
      SEVERITY_FRACTION,
      NEAR_FRACTION,
      QUORUM));

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use(math::const10Function)
      .use(new SymetricNoiseGenerator(MAX_NOISE))
      .after(10).add(-5.0);  // Introduce sudden drop.

  ITERATE_SIGNAL(signalGen) {
    Result<Detection> result =
      AssuranceDropAnalyzer.processSample((*signalGen)());

    if (signalGen.iteration >= 11) {
      EXPECT_SOME(result);
    } else {
      EXPECT_NONE(result);
    }
  }
}


TEST(AssuranceDropAnalyzerTest, NoisyLoadOneBigDropMoreCheckpoints) {
  const uint64_t WINDOWS_SIZE = 16;
  const uint64_t MAX_CHECKPOINTS = 5;
  const double_t QUORUM = 0.70;

  const double_t FRACTION_THRESHOLD = 0.5;
  const double_t SEVERITY_FRACTION = 1;
  const double_t NEAR_FRACTION = 0.1;
  const uint64_t ITERATIONS = 30;
  const uint64_t MAX_NOISE = 4;

  AssuranceDropAnalyzer AssuranceDropAnalyzer(
    Tag(QOS_CONTROLLER, "AssuranceDropAnalyzer"),
    createAssuranceAnalyzerCfg(
      WINDOWS_SIZE,
      MAX_CHECKPOINTS,
      FRACTION_THRESHOLD,
      SEVERITY_FRACTION,
      NEAR_FRACTION,
      QUORUM));

  SignalScenario signalGen =
    SignalScenario(ITERATIONS)
      .use(math::const10Function)
      .use(new SymetricNoiseGenerator(MAX_NOISE))
      .after(10).add(-5.0);  // Introduce sudden drop.

  ITERATE_SIGNAL(signalGen) {
    Result<Detection> result =
      AssuranceDropAnalyzer.processSample((*signalGen)());

    if (signalGen.iteration >= 11) {
      EXPECT_SOME(result);
    } else {
      EXPECT_NONE(result);
    }
  }
}

}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos
