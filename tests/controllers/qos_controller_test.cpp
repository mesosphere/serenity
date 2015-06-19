#include <gtest/gtest.h>

#include <list>

#include <mesos/resources.hpp>

#include <mesos/slave/oversubscription.pb.h> // ONLY USEFUL AFTER RUNNING PROTOC
#include <mesos/slave/qos_controller.hpp>

#include <stout/gtest.hpp>

#include <process/gtest.hpp>

#include "qos_controller/serenity_controller.hpp"

#include "tests/common/serenity.hpp"

using std::list;

using mesos::slave::QoSController;
using mesos::slave::QoSCorrection;

namespace mesos {
namespace serenity {
namespace tests {

// NOTE: For now checking only the interface.
TEST(SerenityControllerTest, NoQoSCorrections)
{
  Try<QoSController*> qoSController =
    serenity::SerenityController::create(None());
  ASSERT_SOME(qoSController);

  QoSController* controller = qoSController.get();

  MockSlaveUsage usage(5);

  Try<Nothing> initialize = controller->initialize(
      lambda::bind(&MockSlaveUsage::usage, &usage));

  process::Future<list<QoSCorrection>> result = controller->corrections();

  AWAIT_READY(result);

  EXPECT_EQ(0u, result.get().size());
}

} // namespace tests {
} // namespace serenity {
} // namespace mesos {

