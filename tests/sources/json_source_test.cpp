#include <gtest/gtest.h>
#include <mesos/mesos.hpp>

#include "tests/common/sinks/dummy_sink.hpp"
#include "tests/common/sources/json_source.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(JsonSource, ProduceRuFromFile) {
  DummySink<ResourceUsage> dummySink;
  JsonSource jsonSource;
  jsonSource.addConsumer(&dummySink);
  jsonSource.RunTests("tests/fixtures/json_source_test.json");

  ASSERT_EQ(dummySink.numberOfMessagesConsumed, 4);
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

