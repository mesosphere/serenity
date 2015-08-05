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
  ValveFilter<Estimator> valveFilter(
      &mockSink,
      true);

  // First component in pipeline.
  MockSource<ResourceUsage> mockSource(&valveFilter);

  // PHASE 1: Run pipeline first time.
  ResourceUsage usage;
  mockSource.produce(usage);

  // Expect that the usage was consumed by sink. (And slack estimated).
  EXPECT_EQ(1, mockSink.numberOfMessagesConsumed);

  // PHASE 2: Disable estimator pipeline.
  UPID upid(getValveProcessBaseName<Estimator>(), address());

  Future<http::Response> response =
    http::post(upid, VALVE_ROUTE + "?" +
                     PIPELINE_ENABLE_KEY + "=false");
  AWAIT_READY(response);

  AWAIT_EXPECT_RESPONSE_STATUS_EQ(http::OK().status, response);

  // Run pipeline second time.
  mockSource.produce(usage);

  // Expect that the usage wasn't consumed by sink (slack wasn't estimated).
  EXPECT_EQ(1, mockSink.numberOfMessagesConsumed);

  // PHASE 3: Enable estimator pipeline.
  response = http::post(upid,
                        VALVE_ROUTE + "?" + PIPELINE_ENABLE_KEY + "=true");

  AWAIT_READY(response);

  AWAIT_EXPECT_RESPONSE_STATUS_EQ(http::OK().status, response);

  // Run pipeline third time.
  mockSource.produce(usage);

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
  ValveFilter<QoS> valveFilter(
      &mockSink,
      true);

  // First component in pipeline.
  MockSource<ResourceUsage> mockSource(&valveFilter);

  // PHASE 1: Run pipeline first time.
  ResourceUsage usage;
  mockSource.produce(usage);

  // Expect that the usage was consumed by sink.
  // It models the situation when IPC Drop won't have data to
  // create new QoS Corrections.
  EXPECT_EQ(1, mockSink.numberOfMessagesConsumed);

  // PHASE 2: Disable Controller pipeline.
  UPID upid(getValveProcessBaseName<QoS>(), address());

  Future<http::Response> response =
      http::post(upid,
                 VALVE_ROUTE + "?" + PIPELINE_ENABLE_KEY + "=false");
  AWAIT_READY(response);

  AWAIT_EXPECT_RESPONSE_STATUS_EQ(http::OK().status, response);

  // Run pipeline second time.
  mockSource.produce(usage);

  // Expect that the usage wasn't consumed by sink (No QoSCorrections).
  EXPECT_EQ(1, mockSink.numberOfMessagesConsumed);

  // PHASE 3: Enable Controller pipeline.
  response = http::post(upid,
                        VALVE_ROUTE + "?" + PIPELINE_ENABLE_KEY + "=true");
  AWAIT_READY(response);

  AWAIT_EXPECT_RESPONSE_STATUS_EQ(http::OK().status, response);

  // Run pipeline third time.
  mockSource.produce(usage);

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
  ValveFilter<QoS> controllerValveFilter(
      &controllerMockSink,
      true);

  // Valve filter which exposes http endpoint for disabling/enabling
  // slack estimations.
  ValveFilter<Estimator> estimatorValveFilter(
      &estimatorMockSink,
      true);

  // First component in pipeline. (Fork for pipeline)
  MockSource<ResourceUsage> mockSource(
      &estimatorValveFilter, &controllerValveFilter);

  // PHASE 1: Run pipeline first time.
  ResourceUsage usage;
  mockSource.produce(usage);

  // Expect that the usage was consumed by both sinks.
  EXPECT_EQ(1, estimatorMockSink.numberOfMessagesConsumed);
  EXPECT_EQ(1, controllerMockSink.numberOfMessagesConsumed);

  // PHASE 2: Disable Controller pipeline.
  UPID controllerUpid(getValveProcessBaseName<QoS>(), address());

  Future<http::Response> response =
      http::post(controllerUpid,
                 VALVE_ROUTE + "?" + PIPELINE_ENABLE_KEY + "=false");
  AWAIT_READY(response);

  AWAIT_EXPECT_RESPONSE_STATUS_EQ(http::OK().status, response);

  // Run pipeline second time.
  mockSource.produce(usage);

  // Expect that the usage was consumed by only Estimator sink.
  EXPECT_EQ(2, estimatorMockSink.numberOfMessagesConsumed);
  EXPECT_EQ(1, controllerMockSink.numberOfMessagesConsumed);

  // PHASE 3: Enable Controller pipeline.
  response = http::post(controllerUpid,
                        VALVE_ROUTE + "?" + PIPELINE_ENABLE_KEY + "=true");
  AWAIT_READY(response);

  AWAIT_EXPECT_RESPONSE_STATUS_EQ(http::OK().status, response);

  UPID estimatorUpid(getValveProcessBaseName<Estimator>(), address());
  // Disable estimator pipeline.
  response = http::post(estimatorUpid,
                        VALVE_ROUTE + "?" + PIPELINE_ENABLE_KEY + "=false");
  AWAIT_READY(response);

  AWAIT_EXPECT_RESPONSE_STATUS_EQ(http::OK().status, response);
  // Run pipeline third time.
  mockSource.produce(usage);

  // Expect that the usage was consumed by only Controller sink.
  EXPECT_EQ(2, estimatorMockSink.numberOfMessagesConsumed);
  EXPECT_EQ(2, controllerMockSink.numberOfMessagesConsumed);
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

