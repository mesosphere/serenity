#include <gtest/gtest.h>
#include <mesos/mesos.hpp>

#include "tests/common/sinks/mock_sink.hpp"
#include "tests/common/sources/json_source.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(JsonSource, ProduceRuFromFile) {

  MockSink<ResourceUsage> sink;
  JsonSource jsonSource;
  jsonSource.addConsumer(&sink);
  jsonSource.RunTests("tests/fixtures/json_source_test.json");

  ASSERT_EQ(sink.numberOfMessagesConsumed, 2);
}

} // namespace tests {
} // namespace serenity {
} // namespace mesos {

