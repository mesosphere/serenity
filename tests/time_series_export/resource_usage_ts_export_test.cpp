#include "tests/common/sources/json_source.hpp"
#include "tests/common/sinks/mock_sink.hpp"

#include "time_series_export/backend/influx_db8.hpp"
#include "time_series_export/resource_usage_ts_export.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(ResourceUsageTimeSeriesExport, BasicTest) {
  InfluxDb8Backend backend("localhost", "8086", "serenity", "root", "root");
  ResourceUsageTimeSeriesExporter ruExporter("tagged-test", backend);
  JsonSource jsonSource;
  MockSink<ResourceUsage> mockSink;

  jsonSource.addConsumer(&ruExporter);
  jsonSource.addConsumer(&mockSink);

  jsonSource.RunTests(
      "tests/fixtures/baseline_smoke_test_resource_usage.json");
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos
