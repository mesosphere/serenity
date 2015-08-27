#include <list>
#include <string>

#include "gtest/gtest.h"

#include "stout/gtest.hpp"

#include "mesos/mesos.hpp"

#include "messages/serenity.hpp"

#include "filters/executor_age.hpp"

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

/**
 * Check if filterPrExecutors function properly filters out PR executors.
 */
TEST(HelperFunctionsTest, filterPrExecutorsEval) {
  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson(QOS_FIXTURE);
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  std::list<ResourceUsage_Executor> ret = filterPrExecutors(usage);

  ASSERT_EQ(3u, ret.size());

  // Expected only BE executors.
  for (auto executor : ret) {
    Resources allocated(executor.allocated());
    EXPECT_FALSE(allocated.revocable().empty());
  }
}


/**
 * QoSCorrectionObserver receiving empty contentions should produce empty
 * correction.
 */
TEST(QoSCorrectionObserverSeniorityDeciderTest, EmptyContentions) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
    .WillOnce(DoAll(
        FutureArg<0>(&corrections),
        Return(Nothing())));

  ExecutorAgeFilter age;

  QoSCorrectionObserver observer(&mockSink, 2, &age);

  age.addConsumer(&observer);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&age);

  // Two fake Contention filters as a source of Contentions.
  MockSource<Contentions> contentionSource1(&observer);
  MockSource<Contentions> contentionSource2(&observer);

  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson(QOS_FIXTURE);
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  usageSource.produce(usage);

  EXPECT_FALSE(corrections.isReady());

  // Producing empty contentions.
  contentionSource1.produce(Contentions());

  EXPECT_FALSE(corrections.isReady());

  // Producing empty contentions.
  contentionSource2.produce(Contentions());

  EXPECT_TRUE(corrections.isReady());

  EXPECT_TRUE(corrections.get().empty());
}


/**
 * QoSCorrectionObserver receiving a contention with aggressor specified
 * from one filter should produce correction for this specified aggressor.
 */
TEST(QoSCorrectionObserverSeniorityDeciderTest,
     OneContentionAggressorSpecified) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&corrections),
          Return(Nothing())));

  ExecutorAgeFilter age;

  QoSCorrectionObserver observer(&mockSink, 2, &age);

  age.addConsumer(&observer);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&age);

  // Two fake Contention filters as a source of Contentions.
  MockSource<Contentions> contentionSource1(&observer);
  MockSource<Contentions> contentionSource2(&observer);

  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson(QOS_FIXTURE);
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  usageSource.produce(usage);

  EXPECT_FALSE(corrections.isReady());

  // Producing one contention with aggressor specified.
  Contention contention;
  contention.set_type(Contention_Type_CPU);
  contention.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_4CPUS).executor_info()));
  contention.mutable_aggressor()->CopyFrom(
      createExecutorWorkID(usage.executors(BE_1CPUS).executor_info()));

  contentionSource1.produce({contention});

  EXPECT_FALSE(corrections.isReady());

  // Producing empty contentions.
  contentionSource2.produce(Contentions());

  EXPECT_TRUE(corrections.isReady());

  // Check correction decision.
  ASSERT_EQ(1u, corrections.get().size());
  EXPECT_EQ(slave::QoSCorrection_Type_KILL, corrections.get().front().type());
  EXPECT_EQ(WID(corrections.get().front().kill()),
            WID(usage.executors(BE_1CPUS).executor_info()));
}


/**
 * QoSCorrectionObserver receiving a contention without aggressor
 * and with small severity specified from one filter should produce a
 * correction for the newest executor.
 */
TEST(QoSCorrectionObserverSeniorityDeciderTest, OneContentionSmallSeverity) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&corrections),
          Return(Nothing())));

  ExecutorAgeFilter age;

  QoSCorrectionObserver observer(
      &mockSink, 2, &age);

  age.addConsumer(&observer);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&age);

  // Two fake Contention filters as a source of Contentions.
  MockSource<Contentions> contentionSource1(&observer);
  MockSource<Contentions> contentionSource2(&observer);

  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson(QOS_FIXTURE);
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  usageSource.produce(usage);

  EXPECT_FALSE(corrections.isReady());

  // Producing one contention with aggressor not specified and
  // small severtiy.
  Contention contention;
  contention.set_type(Contention_Type_CPU);
  contention.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_4CPUS).executor_info()));
  contention.set_severity(0.3);

  contentionSource1.produce({contention});

  EXPECT_FALSE(corrections.isReady());

  // Producing empty contentions.
  contentionSource2.produce(Contentions());

  EXPECT_TRUE(corrections.isReady());

  // Check correction decision.
  ASSERT_EQ(1u, corrections.get().size());
  EXPECT_EQ(slave::QoSCorrection_Type_KILL, corrections.get().front().type());
  EXPECT_EQ(WID(corrections.get().front().kill()),
            WID(usage.executors(BE_1CPUS).executor_info()));
}

}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos
