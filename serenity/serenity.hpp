#ifndef SERENITY_SERENITY_HPP
#define SERENITY_SERENITY_HPP

#include <vector>

#include "glog/logging.h"

#include "stout/nothing.hpp"
#include "stout/try.hpp"

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


/**
 * This consumer requires that all producents will run consume on
 * this instance. (Even during error flow).
 */
template<typename T>
class SyncConsumer : public Consumer<T> {
 public:
  explicit SyncConsumer(uint64_t _producentsToWaitFor)
    : producentsToWaitFor(_producentsToWaitFor) {
    CHECK_ERR(_producentsToWaitFor > 0);
  }

  virtual ~SyncConsumer() {}

  Try<Nothing> consume(const T& in) {
    this->products.push_back(in);

    if (this->products.size() == this->producentsToWaitFor) {
      // Run virtual function which should be implemented in derived
      // class.
      this->_syncConsume(this->products);

      this->products.clear();
    }

    return Nothing();
  }

  virtual Try<Nothing> _syncConsume(const std::vector<T> products) = 0;

 private:
  uint64_t producentsToWaitFor;
  std::vector<T> products;
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
    if (consumer != nullptr) {
      consumers.push_back(consumer);
    } else {
      LOG(ERROR) << "Consumer must not be null.";
    }
    return Nothing();
  }

 protected:
  std::vector<Consumer<T>*> consumers;

  Try<Nothing> produce(T out) {
    for (auto c : consumers) {
      Try<Nothing> ret = c->consume(out);

      // Stop the pipeline in case of error.
      if (ret.isError()) return ret;
    }
    return Nothing();
  }
};


enum ModuleType {
  RESOURCE_ESTIMATOR,
  QOS_CONTROLLER
};


class ResourceEstimatorType {
 public:
  static constexpr const char* const name = "[SerenityEstimator] ";
  static constexpr const ModuleType type = RESOURCE_ESTIMATOR;
};
using Estimator = ResourceEstimatorType;


class QoSControllerType {
 public:
  static constexpr const char* const name = "[SerenityQoSController] ";
  static constexpr const ModuleType type = QOS_CONTROLLER;
};
using QoS = QoSControllerType;

}  // namespace serenity
}  // namespace mesos


#endif  // SERENITY_SERENITY_HPP
