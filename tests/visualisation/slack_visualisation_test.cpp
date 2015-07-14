#include "observers/slack_resource.hpp"

#include "tests/common/sources/json_source.hpp"
#include "tests/common/sinks/mock_sink.hpp"

#include "visualisation/slack_visualisation.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(SlackResourceVisualisation, BasicTest) {
  SlackResourceObserver observer;
  SlackVisualisationFilter slackVisualisation;
  JsonSource jsonSource;
  MockSink<Resources> mockSink;

  jsonSource.addConsumer(&observer);
  observer.addConsumer(&mockSink);
  observer.addConsumer(&slackVisualisation);

  jsonSource.RunTests("tests/fixtures/slack_calculation_test.json");
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos
