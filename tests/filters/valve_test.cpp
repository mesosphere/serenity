#include <gtest/gtest.h>

#include <stout/gtest.hpp>

#include "filters/valve.hpp"

#include "process/future.hpp"
#include "process/http.hpp"
#include "process/pid.hpp"

#include "tests/common/sinks/mock_sink.hpp"
#include "tests/common/sources/mock_source.hpp"

namespace mesos {
namespace serenity {
namespace tests {

// TODO(bplotka): Break into explicit using-declarations.
using namespace process;  // NOLINT(build/namespaces)

using ::testing::DoAll;


TEST(ValveFilterTest, EstimatorDisableThenEnable) {
  // End of pipeline.
  MockSink<ResourceUsage> mockSink;
  EXPECT_CALL(mockSink, consume(_))
    .Times(2);

  // Second component in pipeline.
  // Valve filter which exposes http endpoint for disabling/enabling
  // slack estimations.
  ValveFilter valveFilter(
      &mockSink,
      ValveType::RESOURCE_ESTIMATOR_VALVE,
      true);

  // First component in pipeline.
  MockSource<ResourceUsage> publicSource(&valveFilter);

  // PHASE 1: Run pipeline first time.
  ResourceUsage usage;
  publicSource.produce(usage);

  // Expect that the usage was consumed by sink. (And slack estimated).
  EXPECT_EQ(1, mockSink.numberOfMessagesConsumed);

  // PHASE 2: Disable estimator pipeline.
  UPID upid(getProcessBaseName(RESOURCE_ESTIMATOR_VALVE), address());

  Future<http::Response> response =
    http::post(upid, VALVE_ROUTE + "?" +
                     PIPELINE_ENABLE_KEY + "=false");
  AWAIT_READY(response);

  AWAIT_EXPECT_RESPONSE_STATUS_EQ(http::OK().status, response);

  // Run pipeline second time.
  publicSource.produce(usage);

  // Expect that the usage wasn't consumed by sink (slack wasn't estimated).
  EXPECT_EQ(1, mockSink.numberOfMessagesConsumed);

  // PHASE 3: Enable estimator pipeline.
  response = http::post(upid,
                        VALVE_ROUTE + "?" + PIPELINE_ENABLE_KEY + "=true");

  AWAIT_READY(response);

  AWAIT_EXPECT_RESPONSE_STATUS_EQ(http::OK().status, response);

  // Run pipeline third time.
  publicSource.produce(usage);

  // Slack estimator should continue estimations.
  EXPECT_EQ(2, mockSink.numberOfMessagesConsumed);
}


TEST(ValveFilterTest, ControllerDisableThenEnable) {
  // End of pipeline.
  MockSink<ResourceUsage> mockSink;
  EXPECT_CALL(mockSink, consume(_))
      .Times(2);

  // Second component in pipeline.
  // Valve filter which exposes http endpoint for disabling/enabling
  // QoS Controller assurance.
  ValveFilter valveFilter(
      &mockSink,
      ValveType::QOS_CONTROLLER_VALVE,
      true);

  // First component in pipeline.
  MockSource<ResourceUsage> publicSource(&valveFilter);

  // PHASE 1: Run pipeline first time.
  ResourceUsage usage;
  publicSource.produce(usage);

  // Expect that the usage was consumed by sink.
  // It models the situation when IPC Drop won't have data to
  // create new QoS Corrections.
  EXPECT_EQ(1, mockSink.numberOfMessagesConsumed);

  // PHASE 2: Disable Controller pipeline.
  UPID upid(getProcessBaseName(QOS_CONTROLLER_VALVE), address());

  Future<http::Response> response =
      http::post(upid,
                 VALVE_ROUTE + "?" + PIPELINE_ENABLE_KEY + "=false");
  AWAIT_READY(response);

  AWAIT_EXPECT_RESPONSE_STATUS_EQ(http::OK().status, response);

  // Run pipeline second time.
  publicSource.produce(usage);

  // Expect that the usage wasn't consumed by sink (No QoSCorrections).
  EXPECT_EQ(1, mockSink.numberOfMessagesConsumed);

  // PHASE 3: Enable Controller pipeline.
  response = http::post(upid,
                        VALVE_ROUTE + "?" + PIPELINE_ENABLE_KEY + "=true");
  AWAIT_READY(response);

  AWAIT_EXPECT_RESPONSE_STATUS_EQ(http::OK().status, response);

  // Run pipeline third time.
  publicSource.produce(usage);

  // QoS Controller should continue assuring QoS for oversubscription.
  EXPECT_EQ(2, mockSink.numberOfMessagesConsumed);
}


TEST(ValveFilterTest, ControllerAndEstimatorEndpointsRunningTogether) {
  // End of pipeline for Estimator.
  MockSink<ResourceUsage> estimatorMockSink;
  EXPECT_CALL(estimatorMockSink, consume(_))
      .Times(2);

  // End of pipeline for QoSController
  MockSink<ResourceUsage> controllerMockSink;
  EXPECT_CALL(controllerMockSink, consume(_))
      .Times(2);

  // Valve filter which exposes http endpoint for disabling/enabling
  // QoS Controller assurance.
  ValveFilter controllerValveFilter(
      &controllerMockSink,
      ValveType::QOS_CONTROLLER_VALVE,
      true);

  // Valve filter which exposes http endpoint for disabling/enabling
  // slack estimations.
  ValveFilter estimatorValveFilter(
      &estimatorMockSink,
      ValveType::RESOURCE_ESTIMATOR_VALVE,
      true);

  // First component in pipeline. (Fork for pipeline)
  MockSource<ResourceUsage> publicSource(
      &estimatorValveFilter, &controllerValveFilter);

  // PHASE 1: Run pipeline first time.
  ResourceUsage usage;
  publicSource.produce(usage);

  // Expect that the usage was consumed by both sinks.
  EXPECT_EQ(1, estimatorMockSink.numberOfMessagesConsumed);
  EXPECT_EQ(1, controllerMockSink.numberOfMessagesConsumed);

  // PHASE 2: Disable Controller pipeline.
  UPID controllerUpid(getProcessBaseName(QOS_CONTROLLER_VALVE), address());

  Future<http::Response> response =
      http::post(controllerUpid,
                 VALVE_ROUTE + "?" + PIPELINE_ENABLE_KEY + "=false");
  AWAIT_READY(response);

  AWAIT_EXPECT_RESPONSE_STATUS_EQ(http::OK().status, response);

  // Run pipeline second time.
  publicSource.produce(usage);

  // Expect that the usage was consumed by only Estimator sink.
  EXPECT_EQ(2, estimatorMockSink.numberOfMessagesConsumed);
  EXPECT_EQ(1, controllerMockSink.numberOfMessagesConsumed);

  // PHASE 3: Enable Controller pipeline.
  response = http::post(controllerUpid,
                        VALVE_ROUTE + "?" + PIPELINE_ENABLE_KEY + "=true");
  AWAIT_READY(response);

  AWAIT_EXPECT_RESPONSE_STATUS_EQ(http::OK().status, response);

  UPID estimatorUpid(getProcessBaseName(RESOURCE_ESTIMATOR_VALVE), address());
  // Disable estimator pipeline.
  response = http::post(estimatorUpid,
                        VALVE_ROUTE + "?" + PIPELINE_ENABLE_KEY + "=false");
  AWAIT_READY(response);

  AWAIT_EXPECT_RESPONSE_STATUS_EQ(http::OK().status, response);
  // Run pipeline third time.
  publicSource.produce(usage);

  // Expect that the usage was consumed by only Controller sink.
  EXPECT_EQ(2, estimatorMockSink.numberOfMessagesConsumed);
  EXPECT_EQ(2, controllerMockSink.numberOfMessagesConsumed);
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

