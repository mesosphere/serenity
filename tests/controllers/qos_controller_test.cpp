#include <gtest/gtest.h>

#include <mesos/resources.hpp>

#include <mesos/slave/oversubscription.pb.h>  // ONLY USEFUL AFTER RUNNING PROTOC
#include <mesos/slave/qos_controller.hpp>

#include <stout/gtest.hpp>

#include <process/gtest.hpp>
#include <list>

#include "qos_controller/serenity_controller.hpp"

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

  MockSlaveUsage usage("tests/fixtures/json_source_test.json");

  Try<Nothing> initialize = controller->initialize(
      lambda::bind(&MockSlaveUsage::usage, &usage));

  process::Future<list<QoSCorrection>> result = controller->corrections();

  // So far we did not expect QoSCorrections.
  EXPECT_FALSE(result.isReady());
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

