#include <list>

#include "gtest/gtest.h"


#include "mesos/slave/oversubscription.hpp"

#include "messages/serenity.hpp"

#include "process/clock.hpp"
#include "process/gtest.hpp"

#include "pipeline/cpu_qos_pipeline.hpp"

#include "stout/gtest.hpp"

#include "tests/common/config_helper.hpp"
#include "tests/common/usage_helper.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(CpuQoSPipelineTest, FiltersNotProperlyFed) {
  uint64_t WINDOWS_SIZE = 10;
  uint64_t CONTENTION_COOLDOWN = 10;
  double_t FRATIONAL_THRESHOLD = 0.5;

  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson("tests/fixtures/pipeline/insufficient_metrics.json");
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  SerenityConfig conf;
  conf["Detector"] = createAssuranceDetectorCfg(
    WINDOWS_SIZE, CONTENTION_COOLDOWN, FRATIONAL_THRESHOLD);

  conf.set(ENABLED_VISUALISATION, false);
  conf.set(VALVE_OPENED, true);

  QoSControllerPipeline* pipeline = new CpuQoSPipeline(conf);

  Result<QoSCorrections> corrections = pipeline->run(usage);
  EXPECT_NONE(corrections);

  delete pipeline;
}


// This fixture includes 5 executors:
// - 1 BE <1 CPUS> id 0
// - 2 BE <0.5 CPUS> id 1,2
// - 1 PR <4 CPUS> id 3
// - 1 PR <2 CPUS> id 4
// Iterations:
// 0) 3, 4 has IPC = 1
// 1) 3, 4 has IPC = 1 for stable reference.
const char QOS_PIPELINE_FIXTURE1[] =
    "tests/fixtures/pipeline/qos_no_correction.json";
const int BE_1CPUS = 0;
const int BE_0_5CPUS_1 = 1;
const int BE_0_5CPUS_2 = 2;
const int PR_4CPUS = 3;
const int PR_2CPUS = 4;

TEST(CpuQoSPipelineTest, NoCorrections) {
  uint64_t WINDOWS_SIZE = 10;
  uint64_t CONTENTION_COOLDOWN = 10;
  double_t FRATIONAL_THRESHOLD = 0.5;

  MockSlaveUsage mockSlaveUsage(QOS_PIPELINE_FIXTURE1);

  SerenityConfig conf;
  conf["Detector"] = createAssuranceDetectorCfg(
    WINDOWS_SIZE, CONTENTION_COOLDOWN, FRATIONAL_THRESHOLD);

  conf.set(ENABLED_VISUALISATION, false);
  conf.set(VALVE_OPENED, true);

  QoSControllerPipeline* pipeline = new CpuQoSPipeline(conf);

  // First iteration.
  Result<QoSCorrections> corrections =
      pipeline->run(mockSlaveUsage.usage().get());
  EXPECT_NONE(corrections);

  // Second iteration.
  corrections = pipeline->run(mockSlaveUsage.usage().get());
  EXPECT_SOME(corrections);

  EXPECT_TRUE(corrections.get().empty());

  delete pipeline;
}


// This fixture includes 5 executors:
// - 1 BE <1 CPUS> id 0
// - 2 BE <0.5 CPUS> id 1,2
// - 1 PR <4 CPUS> id 3
// - 1 PR <2 CPUS> id 4
// Iterations:
// 0) 3, 4 have IPC = 1
// 1) 3, 4 have IPC = 1 for stable reference.
// 2) 3 has IPC 0.5, 4 has IPC 1
const char QOS_PIPELINE_FIXTURE2[] =
    "tests/fixtures/pipeline/qos_one_drop_correction.json";

TEST(QoSIpcPipelineTest, AssuranceDetectorOneDropCorrectionsNoEma) {
  uint64_t WINDOWS_SIZE = 10;
  uint64_t CONTENTION_COOLDOWN = 10;
  double_t FRATIONAL_THRESHOLD = 0.4;

  MockSlaveUsage mockSlaveUsage(QOS_PIPELINE_FIXTURE2);

  SerenityConfig conf;
  conf["Detector"] = createAssuranceDetectorCfg(
    WINDOWS_SIZE, CONTENTION_COOLDOWN, FRATIONAL_THRESHOLD);
  conf.set(ema::ALPHA,  1);  // Alpha = 1 means no smoothing.
  conf.set(ENABLED_VISUALISATION, false);
  conf.set(VALVE_OPENED, true);

  QoSControllerPipeline* pipeline = new CpuQoSPipeline(conf);

  // First iteration.
  Result<QoSCorrections> corrections =
      pipeline->run(mockSlaveUsage.usage().get());
  EXPECT_NONE(corrections);

  ResourceUsage usage = mockSlaveUsage.usage().get();
  const int32_t LOAD_ITERATIONS = 12;
  LoadGenerator loadGen(
      [](double_t iter) { return 1; },
      new ZeroNoise(),
      LOAD_ITERATIONS);

  for (; loadGen.end(); loadGen++) {
    // Test scenario: After 10 iterations create drop in
    // IPC for executor num 3.
    double_t ipcFor3Executor = (*loadGen)();
    if (loadGen.iteration >= 11) {
      ipcFor3Executor /= 2.0;
    }

    usage.mutable_executors(PR_4CPUS)->CopyFrom(
      generateIPC(usage.executors(PR_4CPUS),
                  ipcFor3Executor,
                  (*loadGen).timestamp));

    usage.mutable_executors(PR_2CPUS)->CopyFrom(
      generateIPC(usage.executors(PR_2CPUS),
                  (*loadGen)(),
                  (*loadGen).timestamp));
    // Third iteration (repeated).
    corrections = pipeline->run(usage);
    if (loadGen.iteration >= 11) {
      EXPECT_SOME(corrections);
      ASSERT_EQ(slave::QoSCorrection_Type_KILL,
                corrections.get().front().type());
      // Make sure that we do not kill PR tasks!
      EXPECT_NE("serenityPR",
                corrections.get().front().kill().executor_id().value());
      EXPECT_NE("serenityPR2",
                corrections.get().front().kill().executor_id().value());
    } else {
      EXPECT_SOME(corrections);
      EXPECT_TRUE(corrections.get().empty());
    }
  }

  delete pipeline;
}


TEST(QoSIpcPipelineTest, AssuranceDetectorOneDropCorrectionsWithEma) {
  uint64_t WINDOWS_SIZE = 10;
  uint64_t CONTENTION_COOLDOWN = 5;
  double_t FRATIONAL_THRESHOLD = 0.3;
  double_t SEVERITY_LEVEL = 0.1;
  double_t NEAR_LEVEL = 0.1;

  MockSlaveUsage mockSlaveUsage(QOS_PIPELINE_FIXTURE2);

  SerenityConfig conf;
  conf["Detector"] = createAssuranceDetectorCfg(
    WINDOWS_SIZE,
    CONTENTION_COOLDOWN,
    FRATIONAL_THRESHOLD,
    SEVERITY_LEVEL,
    NEAR_LEVEL);
  conf.set(ema::ALPHA, 0.9);
  conf.set(ENABLED_VISUALISATION, false);
  conf.set(VALVE_OPENED, true);

  QoSControllerPipeline* pipeline = new CpuQoSPipeline(conf);

  // First iteration.
  Result<QoSCorrections> corrections =
      pipeline->run(mockSlaveUsage.usage().get());
  EXPECT_NONE(corrections);

  ResourceUsage usage = mockSlaveUsage.usage().get();
  const int32_t LOAD_ITERATIONS = 17;
  LoadGenerator loadGen(
    [](double_t iter) { return 1; },
    new ZeroNoise(),
    LOAD_ITERATIONS);

  for (; loadGen.end(); loadGen++) {
    // Test scenario: After 10 iterations create drop in IPC for
    // executor num 3.
    double_t ipcFor3Executor = (*loadGen)();
    if (loadGen.iteration >= 11 && loadGen.iteration <= 14) {
      ipcFor3Executor /= 2.0;
    }

    usage.mutable_executors(PR_4CPUS)->CopyFrom(
      generateIPC(usage.executors(PR_4CPUS),
                  ipcFor3Executor,
                  (*loadGen).timestamp));

    usage.mutable_executors(PR_2CPUS)->CopyFrom(
      generateIPC(usage.executors(PR_2CPUS),
                  (*loadGen)(),
                  (*loadGen).timestamp));
    // Third iteration (repeated).
    corrections = pipeline->run(usage);

    // Assurance Detector will wait for signal to be returned to the
    // established state.
    if (loadGen.iteration == 11) {
      EXPECT_SOME(corrections);
      ASSERT_EQ(slave::QoSCorrection_Type_KILL,
                corrections.get().front().type());
      // Make sure that we do not kill PR tasks!
      EXPECT_NE("serenityPR",
                corrections.get().front().kill().executor_id().value());
      EXPECT_NE("serenityPR2",
                corrections.get().front().kill().executor_id().value());
    } else {
      EXPECT_SOME(corrections);
      EXPECT_TRUE(corrections.get().empty());
    }
  }

  delete pipeline;
}


TEST(QoSIpcPipelineTest, AssuranceDetectorTwoDropCorrectionsWithEma) {
  uint64_t WINDOWS_SIZE = 10;
  uint64_t CONTENTION_COOLDOWN = 4;
  double_t FRATIONAL_THRESHOLD = 0.3;
  double_t SEVERITY_LEVEL = 1;
  double_t NEAR_LEVEL = 0.1;

  MockSlaveUsage mockSlaveUsage(QOS_PIPELINE_FIXTURE2);

  SerenityConfig conf;
  conf["Detector"] = createAssuranceDetectorCfg(
    WINDOWS_SIZE,
    CONTENTION_COOLDOWN,
    FRATIONAL_THRESHOLD,
    SEVERITY_LEVEL,
    NEAR_LEVEL);
  conf.set(ema::ALPHA, 0.9);
  conf.set(ENABLED_VISUALISATION, false);
  conf.set(VALVE_OPENED, true);

  QoSControllerPipeline* pipeline = new CpuQoSPipeline(conf);

  // First iteration.
  Result<QoSCorrections> corrections =
      pipeline->run(mockSlaveUsage.usage().get());
  EXPECT_NONE(corrections);

  ResourceUsage usage = mockSlaveUsage.usage().get();
  const int32_t LOAD_ITERATIONS = 17;
  LoadGenerator loadGen(
      [](double_t iter) { return 1; },
      new ZeroNoise(),
      LOAD_ITERATIONS);

  for (; loadGen.end(); loadGen++) {
    // Test scenario: After 10 iterations create drop in IPC
    // for executor num 3.
    double_t ipcFor3Executor = (*loadGen)();
    if (loadGen.iteration >= 11) {
      ipcFor3Executor /= 2.0;
    }

    usage.mutable_executors(PR_4CPUS)->CopyFrom(
        generateIPC(usage.executors(PR_4CPUS),
                    ipcFor3Executor,
                    (*loadGen).timestamp));

    usage.mutable_executors(PR_2CPUS)->CopyFrom(
        generateIPC(usage.executors(PR_2CPUS),
                    (*loadGen)(),
                    (*loadGen).timestamp));
    // Third iteration (repeated).
    corrections = pipeline->run(usage);

    // Assurance Detector will wait for signal to be returned to the
    // established state.
    if (loadGen.iteration == 11 || loadGen.iteration == 16) {
      EXPECT_SOME(corrections);
      ASSERT_EQ(slave::QoSCorrection_Type_KILL,
                corrections.get().front().type());
      // Make sure that we do not kill PR tasks!
      EXPECT_NE("serenityPR",
                corrections.get().front().kill().executor_id().value());
      EXPECT_NE("serenityPR2",
                corrections.get().front().kill().executor_id().value());
    } else {
      EXPECT_SOME(corrections);
      EXPECT_TRUE(corrections.get().empty());
    }
  }

  delete pipeline;
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

