#ifndef SERENITY_SERENITY_HPP
#define SERENITY_SERENITY_HPP

#include <stout/try.hpp>
#include <stout/nothing.hpp>

#include <mesos/scheduler/scheduler.hpp>

#include <cstdlib>
#include <vector>

namespace mesos {
namespace serenity {

// The bus socket allows peers to communicate (subscribe and publish)
// asynchronously.
class BusSocket {
};


template<typename T>
class Consumer : public BusSocket {
 public:
  virtual ~Consumer() {}

  virtual Try<Nothing> consume(const T& in) = 0;
};


template<typename T>
class Producer : public BusSocket {
 public:
  Producer() {}

  explicit Producer(Consumer<T>* consumer) {
    addConsumer(consumer);
  }

  explicit Producer(std::vector<Consumer<T>*> consumers_)
    : consumers(consumers_) {}

  virtual ~Producer() {}

  Try<Nothing> addConsumer(Consumer<T>* consumer) {
    consumers.push_back(consumer);
    return Nothing();
  }

 protected:
  std::vector<Consumer<T>*> consumers;

  Try<Nothing> produce(T out) {
    for (auto c : consumers) {
      c->consume(out);
    }
    return Nothing();
  }
};

}  // namespace serenity
}  // namespace mesos


#endif  // SERENITY_SERENITY_HPP
