#include <list>
#include <string>

#include "gtest/gtest.h"

#include "stout/gtest.hpp"

#include "mesos/mesos.hpp"

#include "messages/serenity.hpp"

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


TEST(QoSCorrectionObserverSeverityCpuTest, EmptyContentions) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
    .WillOnce(DoAll(
        FutureArg<0>(&corrections),
        Return(Nothing())));

  QoSCorrectionObserver observer(
      &mockSink, 2, ContentionInterpreters::severityBasedCpuContention);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&observer);

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


TEST(QoSCorrectionObserverSeverityCpuTest, OneContentionAggressorSpecified) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&corrections),
          Return(Nothing())));

  QoSCorrectionObserver observer(
      &mockSink, 2, ContentionInterpreters::severityBasedCpuContention);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&observer);

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
  EXPECT_TRUE(WID(corrections.get().front().kill())
              == WID(usage.executors(BE_1CPUS).executor_info()));
}


TEST(QoSCorrectionObserverSeverityCpuTest, OneContentionSeverityNotSpecified) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&corrections),
          Return(Nothing())));

  QoSCorrectionObserver observer(
      &mockSink, 2, ContentionInterpreters::severityBasedCpuContention);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&observer);

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

  // Producing one contention with aggressor not specified.
  Contention contention;
  contention.set_type(Contention_Type_CPU);
  contention.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_4CPUS).executor_info()));

  contentionSource1.produce({contention});

  EXPECT_FALSE(corrections.isReady());

  // Producing empty contentions.
  contentionSource2.produce(Contentions());

  EXPECT_TRUE(corrections.isReady());

  // Check correction decision.
  ASSERT_EQ(1u, corrections.get().size());
  EXPECT_EQ(slave::QoSCorrection_Type_KILL, corrections.get().front().type());
  EXPECT_TRUE(WID(corrections.get().front().kill())
              == WID(usage.executors(BE_1CPUS).executor_info()));
}


TEST(QoSCorrectionObserverSeverityCpuTest, OneContentionSmallSeverity) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&corrections),
          Return(Nothing())));

  QoSCorrectionObserver observer(
      &mockSink, 2, ContentionInterpreters::severityBasedCpuContention);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&observer);

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
  // small severiy.
  Contention contention;
  contention.set_type(Contention_Type_CPU);
  contention.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_4CPUS).executor_info()));
  contention.set_severity(1.0);

  contentionSource1.produce({contention});

  EXPECT_FALSE(corrections.isReady());

  // Producing empty contentions.
  contentionSource2.produce(Contentions());

  EXPECT_TRUE(corrections.isReady());

  // Check correction decision.
  ASSERT_EQ(1u, corrections.get().size());
  EXPECT_EQ(slave::QoSCorrection_Type_KILL, corrections.get().front().type());
  EXPECT_TRUE(WID(corrections.get().front().kill())
              == WID(usage.executors(BE_1CPUS).executor_info()));
}


TEST(QoSCorrectionObserverSeverityCpuTest, OneContentionBigSeverity) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&corrections),
          Return(Nothing())));

  QoSCorrectionObserver observer(
      &mockSink, 2, ContentionInterpreters::severityBasedCpuContention);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&observer);

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
  // big severiy. (need two be executors to compensate contention)
  Contention contention;
  contention.set_type(Contention_Type_CPU);
  contention.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_4CPUS).executor_info()));
  contention.set_severity(1.3);

  contentionSource1.produce({contention});

  EXPECT_FALSE(corrections.isReady());

  // Producing empty contentions.
  contentionSource2.produce(Contentions());

  EXPECT_TRUE(corrections.isReady());

  QoSCorrections qoSCorrections = corrections.get();
  // Check correction decision.
  ASSERT_EQ(2u, qoSCorrections.size());

  EXPECT_EQ(slave::QoSCorrection_Type_KILL, qoSCorrections.front().type());
  EXPECT_TRUE(WID(qoSCorrections.front().kill())
              == WID(usage.executors(BE_1CPUS).executor_info()));

  qoSCorrections.pop_front();

  EXPECT_EQ(slave::QoSCorrection_Type_KILL, qoSCorrections.front().type());
  EXPECT_TRUE(WID(qoSCorrections.front().kill())
              == WID(usage.executors(BE_0_5CPUS_1).executor_info()));
}


TEST(QoSCorrectionObserverSeverityCpuTest, OneContentionCriticalSeverity) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&corrections),
          Return(Nothing())));

  QoSCorrectionObserver observer(
      &mockSink, 2, ContentionInterpreters::severityBasedCpuContention);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&observer);

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
  // critical severiy. (need three be executors to compensate contention)
  Contention contention;
  contention.set_type(Contention_Type_CPU);
  contention.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_4CPUS).executor_info()));
  contention.set_severity(4);

  contentionSource1.produce({contention});

  EXPECT_FALSE(corrections.isReady());

  // Producing empty contentions.
  contentionSource2.produce(Contentions());

  EXPECT_TRUE(corrections.isReady());

  QoSCorrections qoSCorrections = corrections.get();
  // Check correction decision.
  ASSERT_EQ(3u, qoSCorrections.size());

  EXPECT_EQ(slave::QoSCorrection_Type_KILL, qoSCorrections.front().type());
  EXPECT_TRUE(WID(qoSCorrections.front().kill())
              == WID(usage.executors(BE_1CPUS).executor_info()));

  qoSCorrections.pop_front();

  EXPECT_EQ(slave::QoSCorrection_Type_KILL, qoSCorrections.front().type());
  EXPECT_TRUE(WID(qoSCorrections.front().kill())
              == WID(usage.executors(BE_0_5CPUS_1).executor_info()));

  qoSCorrections.pop_front();

  EXPECT_EQ(slave::QoSCorrection_Type_KILL, qoSCorrections.front().type());
  EXPECT_TRUE(WID(qoSCorrections.front().kill())
              == WID(usage.executors(BE_0_5CPUS_2).executor_info()));
}


TEST(QoSCorrectionObserverSeverityCpuTest,
     TwoSmallContentionsSeverityNotSpecified) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&corrections),
          Return(Nothing())));

  QoSCorrectionObserver observer(
      &mockSink, 2, ContentionInterpreters::severityBasedCpuContention);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&observer);

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

  // Producing two contention with aggressors not specified and
  // no severities.
  Contention contention;
  contention.set_type(Contention_Type_CPU);
  contention.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_4CPUS).executor_info()));

  Contention contention2;
  contention2.set_type(Contention_Type_CPU);
  contention2.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_2CPUS).executor_info()));

  contentionSource1.produce({contention, contention2});

  EXPECT_FALSE(corrections.isReady());

  // Producing empty contentions.
  contentionSource2.produce(Contentions());

  EXPECT_TRUE(corrections.isReady());

  QoSCorrections qoSCorrections = corrections.get();
  // Check correction decision.
  ASSERT_EQ(1u, qoSCorrections.size());

  EXPECT_EQ(slave::QoSCorrection_Type_KILL, qoSCorrections.front().type());
  EXPECT_TRUE(WID(qoSCorrections.front().kill())
              == WID(usage.executors(BE_1CPUS).executor_info()));
}


TEST(QoSCorrectionObserverSeverityCpuTest,
     TwoSmallContentionsFromTwoFiltersSeverityNotSpecified) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&corrections),
          Return(Nothing())));

  QoSCorrectionObserver observer(
      &mockSink, 2, ContentionInterpreters::severityBasedCpuContention);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&observer);

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

  // Producing two contention with aggressors not specified and
  // no severities. One contention per one filter.
  Contention contention;
  contention.set_type(Contention_Type_CPU);
  contention.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_4CPUS).executor_info()));

  Contention contention2;
  contention2.set_type(Contention_Type_CPU);
  contention2.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_2CPUS).executor_info()));

  contentionSource1.produce({contention});

  EXPECT_FALSE(corrections.isReady());

  // Producing empty contentions.
  contentionSource2.produce({contention2});

  EXPECT_TRUE(corrections.isReady());

  QoSCorrections qoSCorrections = corrections.get();
  // Check correction decision.
  ASSERT_EQ(1u, qoSCorrections.size());

  EXPECT_EQ(slave::QoSCorrection_Type_KILL, qoSCorrections.front().type());
  EXPECT_TRUE(WID(qoSCorrections.front().kill())
              == WID(usage.executors(BE_1CPUS).executor_info()));
}


TEST(QoSCorrectionObserverSeverityCpuTest,
     TwoSmallContentionsFromTwoFilters) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&corrections),
          Return(Nothing())));

  QoSCorrectionObserver observer(
      &mockSink, 2, ContentionInterpreters::severityBasedCpuContention);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&observer);

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

  // Producing two contention with aggressors not specified and
  // small severities. One contention per one filter.
  Contention contention;
  contention.set_type(Contention_Type_CPU);
  contention.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_4CPUS).executor_info()));
  contention.set_severity(0.5);

  Contention contention2;
  contention2.set_type(Contention_Type_CPU);
  contention2.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_2CPUS).executor_info()));
  contention2.set_severity(0.5);

  contentionSource1.produce({contention});

  EXPECT_FALSE(corrections.isReady());

  // Producing empty contentions.
  contentionSource2.produce({contention2});

  EXPECT_TRUE(corrections.isReady());

  QoSCorrections qoSCorrections = corrections.get();
  // Check correction decision.
  ASSERT_EQ(1u, qoSCorrections.size());

  EXPECT_EQ(slave::QoSCorrection_Type_KILL, qoSCorrections.front().type());
  EXPECT_TRUE(WID(qoSCorrections.front().kill())
              == WID(usage.executors(BE_1CPUS).executor_info()));
}


TEST(QoSCorrectionObserverSeverityCpuTest,
     TwoBigContentionsFromTwoFilters) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&corrections),
          Return(Nothing())));

  QoSCorrectionObserver observer(
      &mockSink, 2, ContentionInterpreters::severityBasedCpuContention);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&observer);

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

  // Producing two contention with aggressors not specified and
  // big severities. One contention per one filter.
  Contention contention;
  contention.set_type(Contention_Type_CPU);
  contention.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_4CPUS).executor_info()));
  contention.set_severity(1.0);

  Contention contention2;
  contention2.set_type(Contention_Type_CPU);
  contention2.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_2CPUS).executor_info()));
  contention2.set_severity(1.0);

  contentionSource1.produce({contention});

  EXPECT_FALSE(corrections.isReady());

  // Producing empty contentions.
  contentionSource2.produce({contention2});

  EXPECT_TRUE(corrections.isReady());

  QoSCorrections qoSCorrections = corrections.get();
  // Check correction decision.
  ASSERT_EQ(3u, qoSCorrections.size());

  EXPECT_EQ(slave::QoSCorrection_Type_KILL, qoSCorrections.front().type());
  EXPECT_TRUE(WID(qoSCorrections.front().kill())
              == WID(usage.executors(BE_1CPUS).executor_info()));

  qoSCorrections.pop_front();

  EXPECT_EQ(slave::QoSCorrection_Type_KILL, qoSCorrections.front().type());
  EXPECT_TRUE(WID(qoSCorrections.front().kill())
              == WID(usage.executors(BE_0_5CPUS_1).executor_info()));

  qoSCorrections.pop_front();

  EXPECT_EQ(slave::QoSCorrection_Type_KILL, qoSCorrections.front().type());
  EXPECT_TRUE(WID(qoSCorrections.front().kill())
              == WID(usage.executors(BE_0_5CPUS_2).executor_info()));
}


TEST(QoSCorrectionObserverSeverityCpuTest,
     TwoOverlapingContentionsFromTwoFilters) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_))
      .WillOnce(DoAll(
          FutureArg<0>(&corrections),
          Return(Nothing())));

  QoSCorrectionObserver observer(
      &mockSink, 2, ContentionInterpreters::severityBasedCpuContention);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&observer);

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

  // Producing two contention of same PR job with aggressors not specified
  // and small severities.
  Contention contention;
  contention.set_type(Contention_Type_CPU);
  contention.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_4CPUS).executor_info()));
  contention.set_severity(1.0);

  Contention contention2;
  contention2.set_type(Contention_Type_CPU);
  contention2.mutable_victim()->CopyFrom(
      createExecutorWorkID(usage.executors(PR_4CPUS).executor_info()));
  contention2.set_severity(0.5);

  contentionSource1.produce({contention});

  EXPECT_FALSE(corrections.isReady());

  // Producing empty contentions.
  contentionSource2.produce({contention2});

  EXPECT_TRUE(corrections.isReady());

  QoSCorrections qoSCorrections = corrections.get();
  // Check correction decision.
  ASSERT_EQ(1u, qoSCorrections.size());

  EXPECT_EQ(slave::QoSCorrection_Type_KILL, qoSCorrections.front().type());
  EXPECT_TRUE(WID(qoSCorrections.front().kill())
              == WID(usage.executors(BE_1CPUS).executor_info()));
}

}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos
