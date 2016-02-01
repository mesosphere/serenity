#include <list>

#include "gtest/gtest.h"

#include "mesos/resources.hpp"

#include "mesos/slave/oversubscription.hpp"
#include "mesos/slave/qos_controller.hpp"

#include "mesos_module/qos_controller/serenity_controller.hpp"

#include "process/clock.hpp"
#include "process/gtest.hpp"

#include "pipeline/qos_pipeline.hpp"

#include "stout/gtest.hpp"

#include "tests/common/usage_helper.hpp"

using std::list;

using mesos::slave::QoSController;
using mesos::slave::QoSCorrection;

namespace mesos {
namespace serenity {
namespace tests {

class TestCorrectionPipeline : public QoSControllerPipeline {
 public:
  TestCorrectionPipeline() {}

  virtual Result<QoSCorrections> run(const ResourceUsage& _product) {
    QoSCorrections corrections;

    ExecutorInfo executorInfo;
    executorInfo.mutable_framework_id()->set_value("Framework1");
    executorInfo.mutable_executor_id()->set_value("Executor1");

    QoSCorrection correction =
      createKillQoSCorrection(createKill(executorInfo));
    corrections.push_back(correction);
    return corrections;
  }
};

/**
 * This tests checks the interface.
 */
TEST(SerenityControllerTest, PipelineIntegration) {
  Try<QoSController*> qoSController =
    serenity::SerenityController::create(
        std::shared_ptr<QoSControllerPipeline>(
            new TestCorrectionPipeline()));
  ASSERT_SOME(qoSController);

  QoSController* controller = qoSController.get();

  MockSlaveUsage usage(
      "tests/fixtures/baseline_smoke_test_resource_usage.json");

  Try<Nothing> initialize = controller->initialize(
      lambda::bind(&MockSlaveUsage::usage, &usage));

  process::Future<list<QoSCorrection>> result = controller->corrections();

  AWAIT_READY(result);

  EXPECT_EQ(1u, result.get().size());

  EXPECT_EQ("Executor1", result.get().front().kill().executor_id().value());
  EXPECT_EQ("Framework1", result.get().front().kill().framework_id().value());
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

