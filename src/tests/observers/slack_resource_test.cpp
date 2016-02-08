#include "gmock/gmock.h"

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "observers/slack_resource.hpp"

#include "stout/try.hpp"
#include "stout/nothing.hpp"

#include "serenity/math_utils.hpp"

#include "tests/common/sources/json_source.hpp"
#include "tests/common/mocks/mock_sink.hpp"

namespace mesos {
namespace serenity {
namespace tests {

using ::testing::DoubleEq;
using ::testing::Eq;
using ::testing::InSequence;
using ::testing::_;

ACTION_P(BasicTestAction, check) {
  Resources resources = arg0;

  if (utils::AlmostZero(check)) {
    EXPECT_TRUE(resources.empty());
  } else {
    EXPECT_FALSE(resources.empty());

    // if is used here, because gmock can not use ASSERT inside ACTION
    if (!resources.empty()) {
      EXPECT_THAT(resources.begin()->scalar().value(), DoubleEq(check));
      EXPECT_THAT(++resources.begin(), Eq(resources.end()));
    }
  }
  return Nothing();
}


TEST(SlackResourceObserver, BasicTest) {
  SlackResourceObserver observer;
  JsonSource jsonSource;
  MockSink<Resources> mockSink;

  jsonSource.addConsumer(&observer);
  observer.addConsumer(&mockSink);

  std::array<double_t, 6> expectedCpus = {0, 0, 2, 4, 1, 3.7};

  {
    InSequence seq;
    for (double_t param : expectedCpus) {
      EXPECT_CALL(mockSink, consume(_)).WillOnce(BasicTestAction(param));
    }
  }
  jsonSource.RunTests(
      "tests/fixtures/slack_estimator/slack_calculation_test.json");
}


TEST(SlackResourceObserver, MaxOversubscriptionFraction) {
  SlackResourceObserver observer;
  JsonSource jsonSource;
  MockSink<Resources> mockSink;

  jsonSource.addConsumer(&observer);
  observer.addConsumer(&mockSink);

  std::array<double_t, 2> expectedCpus = {0.0, 1.6};

  {
    InSequence seq;
    for (double_t param : expectedCpus) {
      EXPECT_CALL(mockSink, consume(_)).WillOnce(BasicTestAction(param));
    }
  }
  jsonSource.RunTests("tests/fixtures/slack_estimator/"
                          "max_oversubscription_fraction_test.json");
}


}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos
