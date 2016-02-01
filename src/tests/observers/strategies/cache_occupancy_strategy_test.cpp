#include "gmock/gmock.h"

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "observers/slack_resource.hpp"

#include "stout/try.hpp"
#include "stout/nothing.hpp"

#include "serenity/math_utils.hpp"

#include "tests/common/sources/json_source.hpp"
#include "tests/common/sinks/mock_sink.hpp"

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

}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos
