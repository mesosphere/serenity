#ifndef SERENITY_TEST_MOCK_SINK_HPP
#define SERENITY_TEST_MOCK_SINK_HPP

#include <process/gmock.hpp>
#include <process/gtest.hpp>

#include <stout/try.hpp>

#include "serenity/serenity.hpp"

using ::testing::_;
using ::testing::Invoke;
using ::testing::DoDefault;
using ::testing::Return;

namespace mesos {
namespace serenity {
namespace tests {

template<typename T>
class MockSink : public Consumer<T> {
 public:
  MockSink() : numberOfMessagesConsumed(0) {
    ON_CALL(*this, consume(_))
      .WillByDefault(InvokeConsume(this));
    EXPECT_CALL(*this, consume(_))
      .WillRepeatedly(DoDefault());
  }

  virtual ~MockSink() {}

  int numberOfMessagesConsumed;

  MOCK_METHOD1_T(consume, Try<Nothing>(const T& in));
};


ACTION_P(InvokeConsume, sink) {
  sink->numberOfMessagesConsumed++;
  return Nothing();
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_TEST_MOCK_SINK_HPP
