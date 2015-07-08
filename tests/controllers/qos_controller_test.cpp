#include <list>

#include "gtest/gtest.h"

#include "mesos/resources.hpp"

// ONLY USEFUL AFTER RUNNING PROTOC
#include "mesos/slave/oversubscription.pb.h"

#include "mesos/slave/qos_controller.hpp"

#include "process/clock.hpp"
#include "process/gtest.hpp"

#include "qos_controller/serenity_controller.hpp"

#include "stout/gtest.hpp"

#include "tests/common/usage_helper.hpp"

using std::list;

using mesos::slave::QoSController;
using mesos::slave::QoSCorrection;

namespace mesos {
namespace serenity {
namespace tests {

// NOTE: For now checking only the interface.
TEST(SerenityControllerTest, NoQoSCorrections) {
  Try<QoSController*> qoSController =
    serenity::SerenityController::create(None());
  ASSERT_SOME(qoSController);

  QoSController* controller = qoSController.get();

  MockSlaveUsage usage(
      "tests/fixtures/baseline_smoke_test_resource_usage.json");

  Try<Nothing> initialize = controller->initialize(
      lambda::bind(&MockSlaveUsage::usage, &usage));

  process::Clock::pause();

  process::Future<list<QoSCorrection>> result = controller->corrections();

  // Wait for internal QosCorrection defers to be done.
  process::Clock::settle();

  // So far we did not expect QoSCorrections.
  EXPECT_FALSE(result.isReady());
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

