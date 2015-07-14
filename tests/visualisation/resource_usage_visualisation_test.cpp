#include "tests/common/sources/json_source.hpp"
#include "tests/common/sinks/mock_sink.hpp"

#include "visualisation/resource_usage_visualisation.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(ResourceUsageVisualisation, BasicTest) {
  ResourceUsageVisualisation resourceVisualisation;
  JsonSource jsonSource;
  MockSink<ResourceUsage> mockSink;

  jsonSource.addConsumer(&resourceVisualisation);
  jsonSource.addConsumer(&mockSink);

  jsonSource.RunTests(
      "tests/fixtures/baseline_smoke_test_resource_usage.json");
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos
