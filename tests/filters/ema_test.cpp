#include <stout/gtest.hpp>

#include <gtest/gtest.h>

#include "filters/ema.hpp"

#include "process/future.hpp"

#include "tests/common/load_generator.hpp"
#include "tests/common/usage_helper.hpp"
#include "tests/common/sinks/mock_sink.hpp"
#include "tests/common/sources/json_source.hpp"
#include "tests/common/sources/mock_source.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using ::testing::DoAll;

TEST(EMATest, SmoothingConstSample) {
  const double_t THRESHOLD = 0.01;
  const int32_t LOAD_ITERATIONS = 100;
  ExponentialMovingAverage ema(
      EMA_REGULAR_SERIES, 0.2);
  LoadGenerator loadGen(
      [](double_t iter) { return 10; }, new ZeroNoise(), LOAD_ITERATIONS);

  for (; loadGen.end() ; loadGen++) {
    double_t result = ema.calculateEMA((*loadGen)(), (*loadGen).timestamp);

    EXPECT_NEAR((*loadGen).clearValue(), result, THRESHOLD);
  }
}


TEST(EMATest, SmoothingNoisyConstSample) {
  const double_t THRESHOLD = 4;
  const double_t MAX_NOISE = 30;
  const int32_t LOAD_ITERATIONS = 100;
  ExponentialMovingAverage ema(
      EMA_REGULAR_SERIES, 0.2);
  LoadGenerator loadGen(
      [](double_t iter) { return 10; },
      new SymetricNoiseGenerator(MAX_NOISE),
      LOAD_ITERATIONS);

  for (; loadGen.end() ; loadGen++) {
    double_t result = ema.calculateEMA((*loadGen)(), (*loadGen).timestamp);

    EXPECT_NEAR((*loadGen).clearValue(), result, THRESHOLD);
  }
}


TEST(EMATest, SmoothingNoisyLinearSample) {
  const double_t THRESHOLD = 11;
  const double_t MAX_NOISE = 50;
  const int32_t LOAD_ITERATIONS = 100;
  ExponentialMovingAverage ema(
      EMA_REGULAR_SERIES, 0.2);
  LoadGenerator loadGen(
      math::linearFunction,
      new SymetricNoiseGenerator(MAX_NOISE),
      LOAD_ITERATIONS);

  for (; loadGen.end() ; loadGen++) {
    double_t result = ema.calculateEMA((*loadGen)(), (*loadGen).timestamp);

    EXPECT_NEAR((*loadGen).clearValue(), result, THRESHOLD);
  }
}


TEST(EMATest, SmoothingNoisySinSample) {
  const double_t THRESHOLD = 7;
  const double_t MAX_NOISE = 50;
  const int32_t LOAD_ITERATIONS = 100;
  ExponentialMovingAverage ema(
      EMA_REGULAR_SERIES, 0.2);
  LoadGenerator loadGen(
      math::sinFunction,
      new SymetricNoiseGenerator(MAX_NOISE),
      LOAD_ITERATIONS);

  for (; loadGen.end() ; loadGen++) {
    double_t result = ema.calculateEMA((*loadGen)(), (*loadGen).timestamp);

    EXPECT_NEAR((*loadGen).clearValue(), result, THRESHOLD);
  }
}


TEST(EMATest, SmoothingNoisySinSampleDrop) {
  const double_t THRESHOLD = 8;
  const double_t MAX_NOISE = 50;
  const double_t DROP = 10;
  const int32_t LOAD_ITERATIONS = 200;
  ExponentialMovingAverage ema(
      EMA_REGULAR_SERIES, 0.2);
  LoadGenerator loadGen(
      math::sinFunction,
      new SymetricNoiseGenerator(MAX_NOISE),
      LOAD_ITERATIONS);

  for (; loadGen.end() ; loadGen++) {
    // Introduce dramatic drop in the middle of the test.
    if (loadGen.iteration == 100) loadGen.modifier -= DROP;

    double_t result = ema.calculateEMA((*loadGen)(), (*loadGen).timestamp);

    EXPECT_NEAR((*loadGen).clearValue(), result, THRESHOLD);
  }
}


TEST(EMATest, SmoothingNoisySinStableDrop) {
  const double_t THRESHOLD = 8;
  const double_t MAX_NOISE = 50;
  const double_t DROP_PROGRES = 0.2;
  const int32_t LOAD_ITERATIONS = 200;
  ExponentialMovingAverage ema(
      EMA_REGULAR_SERIES, 0.2);
  LoadGenerator loadGen(
      math::sinFunction,
      new SymetricNoiseGenerator(MAX_NOISE),
      LOAD_ITERATIONS);

  for (; loadGen.end() ; loadGen++) {
    // Introduce stable drop in the middle of the test..
    if (loadGen.iteration > 100 &&
        loadGen.iteration < 150) loadGen.modifier -= DROP_PROGRES;

    double_t result = ema.calculateEMA((*loadGen)(), (*loadGen).timestamp);

    EXPECT_NEAR((*loadGen).clearValue(), result, THRESHOLD);
  }
}


TEST(EMATest, IpcEMATest) {
  const double_t THRESHOLD = 0.000001;
  const double_t RESULT_EXECUTOR1 = 0.5;
  const double_t RESULT_EXECUTOR2 = 2;
  // End of pipeline.
  MockSink<ResourceUsage> mockSink;
  process::Future<ResourceUsage> usage;
  EXPECT_CALL(mockSink, consume(_))
    .WillOnce(DoAll(
       FutureArg<0>(&usage),
       InvokeConsumePrintIpcEma(&mockSink)));

  // Second component in pipeline.
  EMAFilter ipcEMAFilter(
      &mockSink, usage::getIpc, usage::setEmaIpc, 0.2);

  // First component in pipeline.
  JsonSource jsonSource(&ipcEMAFilter);

  // Start test.
  ASSERT_SOME(jsonSource.RunTests("tests/fixtures/ema/test.json"));

  ASSERT_TRUE(usage.isReady());
  ASSERT_EQ(2u, usage.get().executors().size());

  // Verify IPC Values (from fixtures)
  double_t ipc =
    usage.get().executors(0).statistics().net_tcp_active_connections();
  EXPECT_NEAR(RESULT_EXECUTOR1, ipc, THRESHOLD);

  ipc = usage.get().executors(1).statistics().net_tcp_active_connections();
  EXPECT_NEAR(RESULT_EXECUTOR2, ipc, THRESHOLD);
}


TEST(EMATest, IpcEMATestNoPerf) {
  // End of pipeline.
  MockSink<ResourceUsage> mockSink;
  EXPECT_CALL(mockSink, consume(_))
      .Times(0);

  // Second component in pipeline.
  EMAFilter ipcEMAFilter(
      &mockSink, usage::getIpc, usage::setEmaIpc, 0.2);

  // First component in pipeline.
  JsonSource jsonSource(&ipcEMAFilter);

  // Start test.
  ASSERT_SOME(jsonSource.RunTests(
      "tests/fixtures/ema/insufficient_metrics_test.json"));

  EXPECT_EQ(0, mockSink.numberOfMessagesConsumed);
}


/**
 * In this test we generate load with noise and
 * test the IpcEMAfilter output in every iteration.
 */
TEST(EMATest, IpcEMATestNoisyConstSample) {
  // End of pipeline.
  MockSink<ResourceUsage> mockSink;

  // Second component in pipeline.
  EMAFilter ipcEMAFilter(
      &mockSink, usage::getIpc, usage::setEmaIpc, 0.2);

  // First component in pipeline.
  MockSource<ResourceUsage> source(&ipcEMAFilter);

  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson("tests/fixtures/start_json_test.json");
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  const double_t IPC_VALUE = 10;
  const double_t THRESHOLD = 1.2;
  const double_t MAX_NOISE = 5;
  const int32_t LOAD_ITERATIONS = 100;
  LoadGenerator loadGen(
      [](double_t iter) { return 10; },
      new SymetricNoiseGenerator(MAX_NOISE),
      LOAD_ITERATIONS);

  for (; loadGen.end(); loadGen++) {
    usage.mutable_executors(0)->CopyFrom(
        generateIPC(usage.executors(0),
                    (*loadGen)(),
                    (*loadGen).timestamp));

    // Run pipeline iteration
    source.produce(usage);

    if (loadGen.iteration > 0)
      mockSink.expectIpc(0, IPC_VALUE, THRESHOLD);
  }

  EXPECT_EQ(99, mockSink.numberOfMessagesConsumed);
}


TEST(EMATest, CpuUsageEMATest) {
  const double_t THRESHOLD = 0.000001;
  const double_t RESULT_EXECUTOR1 = 0.1;
  const double_t RESULT_EXECUTOR2 = 0;

  // End of pipeline.
  MockSink<ResourceUsage> mockSink;
  process::Future<ResourceUsage> usage;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&usage),
          InvokeConsumePrintCPuUsageEma(&mockSink)));

  // Second component in pipeline.
  EMAFilter cpuUsageEMAFilter(
      &mockSink, usage::getCpuUsage, usage::setEmaCpuUsage);

  // First component in pipeline.
  JsonSource jsonSource(&cpuUsageEMAFilter);

  // Start test.
  ASSERT_SOME(jsonSource.RunTests("tests/fixtures/ema/test.json"));

  ASSERT_TRUE(usage.isReady());
  ASSERT_EQ(2u, usage.get().executors().size());

  // Verify CpuUsage Values (from fixtures)
  double_t cpuUsage =
    usage.get().executors(0).statistics().net_tcp_time_wait_connections();
  EXPECT_NEAR(RESULT_EXECUTOR1, cpuUsage, THRESHOLD);

  cpuUsage =
    usage.get().executors(1).statistics().net_tcp_time_wait_connections();
  EXPECT_NEAR(RESULT_EXECUTOR2, cpuUsage, THRESHOLD);
}


TEST(EMATest, CpuUsageEMATestNoCpuStatistics) {
  // End of pipeline.
  MockSink<ResourceUsage> mockSink;
  EXPECT_CALL(mockSink, consume(_))
      .Times(0);

  // Second component in pipeline.
  EMAFilter cpuUsageEMAFilter(
      &mockSink, usage::getCpuUsage, usage::setEmaCpuUsage);

  // First component in pipeline.
  JsonSource jsonSource(&cpuUsageEMAFilter);

  // Start test.
  ASSERT_SOME(jsonSource.RunTests(
      "tests/fixtures/ema/insufficient_metrics_test.json"));

  EXPECT_EQ(0, mockSink.numberOfMessagesConsumed);
}


/**
 * In this test we generate load with noise and
 * test the CpuUsageEMAfilter output in every iteration.
 */
TEST(EMATest, CpuUsageEMATestNoisyConstSample) {
  // End of pipeline.
  MockSink<ResourceUsage> mockSink;

  // Second component in pipeline.
  EMAFilter cpuUsageEMAFilter(
      &mockSink, usage::getCpuUsage, usage::setEmaCpuUsage);

  // First component in pipeline.
  MockSource<ResourceUsage> source(&cpuUsageEMAFilter);

  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson("tests/fixtures/start_json_test.json");
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  const double_t IPC_VALUE = 10;
  const double_t THRESHOLD = 1.2;
  const double_t MAX_NOISE = 5;
  const int32_t LOAD_ITERATIONS = 100;
  LoadGenerator loadGen(
      [](double_t iter) { return 10; },
      new SymetricNoiseGenerator(MAX_NOISE),
      LOAD_ITERATIONS);

  uint64_t previousLoad = 0;
  for (; loadGen.end(); loadGen++) {
    usage.mutable_executors(0)->mutable_statistics()
        ->set_cpus_system_time_secs(previousLoad + (uint64_t)(*loadGen)());
    previousLoad += (uint64_t)(*loadGen)();

    usage.mutable_executors(0)->mutable_statistics()
        ->set_cpus_user_time_secs(0);

    usage.mutable_executors(0)->mutable_statistics()
        ->set_timestamp((*loadGen).timestamp);

    // Run pipeline iteration
    source.produce(usage);

    if (loadGen.iteration > 0)
      mockSink.expectCpuUsage(0, IPC_VALUE, THRESHOLD);
  }

  EXPECT_EQ(99, mockSink.numberOfMessagesConsumed);
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

