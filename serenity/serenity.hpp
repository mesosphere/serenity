#ifndef SERENITY_SERENITY_HPP
#define SERENITY_SERENITY_HPP

#include <cstdlib>

#include <stout/try.hpp>
#include <stout/nothing.hpp>

#include <mesos/scheduler/scheduler.hpp>


namespace mesos {
namespace serenity {

// The bus socket allows peers to communicate (subscribe and publish)
// asynchronously.
class BusSocket {
public:
  // Filters need to claim a topic before being able to
  // publish to it.
  Try<Nothing> registration(std::string topic);

  // NOTE: Subscribe can return a future instead of relying
  // on a provided callback.
  Try<Nothing> subscribe(
      std::string topic,
      std::function<void(mesos::scheduler::Event)> callback); //skonefal TODO: Waiting for serenity.proto generic serenity event

  Try<Nothing> publish(std::string topic, mesos::scheduler::Event event); //skonefal TODO: Waiting for serenity.proto generic serenity event
};


template<typename T>
class Consumer : public BusSocket
{
public:
  virtual ~Consumer() {}

  virtual Try<Nothing> consume(T in) = 0;
};

template<typename T>
class Producer : public BusSocket
{
public:
  Producer() {}

  virtual ~Producer() {}

  Try<Nothing> addConsumer(Consumer<T>* consumer)
  {
    consumers.push_back(consumer);
    return Nothing();
  }

protected:
  std::vector<Consumer<T>*> consumers;

  Try<Nothing> produce(T out)
  {
    for (auto c : consumers) {
      c->consume(out);
    }
    return Nothing();
  }

};

} // namespace serenity
} // namespace mesos


#endif //SERENITY_SERENITY_HPP
