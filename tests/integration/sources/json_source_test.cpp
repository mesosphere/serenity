#include <gtest/gtest.h>
#include <mesos/mesos.hpp>

#include "tests/helpers/sinks/dummy_sink.hpp"
#include "tests/helpers/sources/json_source.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(JsonSource, ProduceRuFromFile) {

  DummySink<ResourceUsage> dummySink;
  JsonSource jsonSource;
  jsonSource.addConsumer(&dummySink);
  //TODO: add target in cmake to copy fixtures to build folder
  jsonSource.RunTests("../tests/fixtures/json_source_ut.json");

  ASSERT_EQ(dummySink.numberOfMessagesConsumed, 2);
}

}
}
}
