#ifndef SERENITY_DUMMYRESOURCEUSAGESINK_H
#define SERENITY_DUMMYRESOURCEUSAGESINK_H

#include <stout/try.hpp>

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

template<typename T>
class DummySink : public Consumer<T>
{
public:

  DummySink<T>() : numberOfMessagesConsumed(0){};

  Try<Nothing> consume(T& in)
  {
    this->numberOfMessagesConsumed++;
    return Nothing();
  }

  int numberOfMessagesConsumed;
};

} // namespace serenity
} // namespace mesos

#endif //SERENITY_DUMMY_RESOURCE_USAGE_SINK_HPP
