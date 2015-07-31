#include "observers/slack_resource.hpp"

#include "tests/common/sources/json_source.hpp"
#include "tests/common/sinks/mock_sink.hpp"

#include "time_series_export/slack_ts_export.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(SlackResourceTimeSeriesExportTest, BasicTest) {
  SlackResourceObserver observer;
  SlackTimeSeriesExporter slackExporter("tagged-test");
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
