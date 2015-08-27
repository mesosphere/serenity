#ifndef SERENITY_MOCK_SOURCE_HPP
#define SERENITY_MOCK_SOURCE_HPP

#include <mesos/mesos.hpp>

#include <stout/try.hpp>

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {
namespace tests {

template<typename T>
class MockSource : public Producer<T> {
 public:
  MockSource() {}

  explicit MockSource(Consumer<T>* _consumer) {
    Producer<T>::addConsumer(_consumer);
  }

  explicit MockSource(Consumer<T>* _consumer,
                        Consumer<T>* _consumer2) {
    Producer<T>::addConsumer(_consumer);
    Producer<T>::addConsumer(_consumer2);
  }

  Try<Nothing> produce(T out) {
    Producer<T>::produce(out);

    return Nothing();
  }
};

}  // namespace tests
}  // namespace serenity
}  // namespace mesos


#endif  // SERENITY_MOCK_SOURCE_HPP
