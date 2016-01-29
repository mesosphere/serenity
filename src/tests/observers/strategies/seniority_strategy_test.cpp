#include <list>
#include <string>

#include "gtest/gtest.h"

#include "stout/gtest.hpp"

#include "mesos/mesos.hpp"

#include "messages/serenity.hpp"

#include "filters/executor_age.hpp"

#include "observers/qos_correction.hpp"

#include "serenity/resource_helper.hpp"

#include "tests/common/usage_helper.hpp"
#include "tests/common/sinks/mock_sink.hpp"
#include "tests/common/sources/mock_source.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using ::testing::DoAll;

// This fixture includes 5 executors:
// - 1 BE <1 CPUS> id 0
// - 2 BE <0.5 CPUS> id 1,2
// - 1 PR <4 CPUS> id 3
// - 1 PR <2 CPUS> id 4
const char QOS_FIXTURE[] = "tests/fixtures/qos/average_usage.json";
const int BE_1CPUS = 0;

/**
 * QoSCorrectionObserver receiving empty contentions should produce empty
 * correction.
 *
 * TODO(skonefal): Refactor it to really test only strategy.
 */
TEST(QoSCorrectionObserverSeniorityDeciderTest, EmptyContentions) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_)).WillOnce(DoAll(
    FutureArg<0>(&corrections),
    Return(Nothing())));

  ExecutorAgeFilter age;

  QoSCorrectionObserver observer(
  &mockSink, 2, &age, new SeniorityStrategy(SerenityConfig()));

  age.addConsumer(&observer);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&age);

  // Two fake Contention filters as a source of Contentions.
  MockSource<Contentions> contentionSource1(&observer);
  MockSource<Contentions> contentionSource2(&observer);

  Try<mesos::FixtureResourceUsage> usages = JsonUsage::ReadJson(QOS_FIXTURE);
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
 * QoSCorrectionObserver receiving a contention without aggressor
 * and with small severity specified from one filter should produce a
 * correction for the newest executor.
 *
 * TODO(skonefal): Refactor it to really test only strategy.
 */
TEST(QoSCorrectionObserverSeniorityDeciderTest, OneContentionSmallSeverity) {
  // End of pipeline for QoSController.
  MockSink<QoSCorrections> mockSink;
  process::Future<QoSCorrections> corrections;
  EXPECT_CALL(mockSink, consume(_)).
    WillOnce(DoAll(FutureArg<0>(&corrections),
                   Return(Nothing())));

  ExecutorAgeFilter age;

  QoSCorrectionObserver observer(
  &mockSink, 2, &age, new SeniorityStrategy(SerenityConfig()));

  age.addConsumer(&observer);

  // Fake slave ResourceUsage source.
  MockSource<ResourceUsage> usageSource(&age);

  // Two fake Contention filters as a source of Contentions.
  MockSource<Contentions> contentionSource1(&observer);
  MockSource<Contentions> contentionSource2(&observer);

  Try<mesos::FixtureResourceUsage> usages = JsonUsage::ReadJson(QOS_FIXTURE);
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
