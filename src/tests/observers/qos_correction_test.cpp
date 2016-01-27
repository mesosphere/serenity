#include <list>
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "stout/gtest.hpp"

#include "mesos/mesos.hpp"

#include "messages/serenity.hpp"

#include "filters/executor_age.hpp"

#include "observers/qos_correction.hpp"

#include "serenity/resource_helper.hpp"
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

class MockQosController : public QoSCorrectionObserver {
 public:
  explicit MockQosController(size_t _syncConsumers) :
    QoSCorrectionObserver(nullptr, _syncConsumers) {}

  MOCK_METHOD0(emptyContentionsReceived, void());
  MOCK_METHOD0(doQosDecision, void());
};

class MockQosRevocationStrategy : public RevocationStrategy {
 public:
  MockQosRevocationStrategy() :
    RevocationStrategy(Tag(QOS_CONTROLLER, "Mocked")) {}

  MOCK_METHOD3(decide, Try<QoSCorrections>(ExecutorAgeFilter* exeutorAge,
                                           const Contentions& contentions,
                                           const ResourceUsage& usage));
};

/**
 * Check if getRevocableExecutors function properly filters out PR executors.
 */
TEST(QosControllerTest, emptyContentionsReceived) {
  MockQosController qosController(1);
  MockQosRevocationStrategy qosStrategy;

  const ResourceUsage usage;
  std::vector<Contentions> syncContenions;

  qosController.consume(usage);
  qosController.syncConsume(syncContenions);

//  EXPECT_CALL(qosController, doQosDecision());
//  EXPECT_CALL(qosController, emptyContentionsReceived());
}

}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos
