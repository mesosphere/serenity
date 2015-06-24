#ifndef SERENITY_DUMMY_SINK_HPP
#define SERENITY_DUMMY_SINK_HPP

#include <stout/try.hpp>

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {
namespace tests {

template<typename T>
class DummySink : public Consumer<T>
{
 public:
  DummySink<T>() : numberOfMessagesConsumed(0) {}

  Try<Nothing> consume(const T& in)
  {
    this->numberOfMessagesConsumed++;
    return Nothing();
  }

  uint32_t numberOfMessagesConsumed;
};

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

#endif //SERENITY_DUMMY_SINK_HPP
