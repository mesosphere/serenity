#include <gtest/gtest.h>

#include <mesos/mesos.hpp>

#include "observers/slack_resource.hpp"

#include "tests/common/serenity.hpp"
#include "tests/common/sinks/printer_sink.hpp"
#include "tests/common/sources/json_source.hpp"
#include "tests/observers/slack_resource_test.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(SlackResourceObserer, BasicTest) {
  SlackResourceObserver observer;
  JsonSource jsonSource;
  PrinterSink<Resource> dummySink;

  jsonSource.addConsumer(&observer);
  observer.addConsumer(&dummySink);

  jsonSource.RunTests("tests/fixtures/json_source_test2.json");

  ASSERT_EQ(dummySink.numberOfMessagesConsumed, 4);


}

} // namespace tests {
} // namespace serenity {
} // namespace mesos {
