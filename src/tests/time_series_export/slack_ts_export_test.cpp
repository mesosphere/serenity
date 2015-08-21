#include "observers/slack_resource.hpp"

#include "tests/common/sources/json_source.hpp"
#include "tests/common/sinks/mock_sink.hpp"

#include "time_series_export/backend/influx_db8.hpp"
#include "time_series_export/slack_ts_export.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(SlackResourceTimeSeriesExportTest, BasicTest) {
  SlackResourceObserver observer;
  InfluxDb8Backend backend("localhost", "8086", "serenity", "root", "root");
  SlackTimeSeriesExporter slackExporter("tagged-test", backend);
  JsonSource jsonSource;
  MockSink<Resources> mockSink;

  jsonSource.addConsumer(&observer);
  observer.addConsumer(&mockSink);
  observer.addConsumer(&slackExporter);

  jsonSource.RunTests("tests/fixtures/slack_calculation_test.json");
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos
