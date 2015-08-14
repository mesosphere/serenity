#ifndef SERENITY_SERENITY_HPP
#define SERENITY_SERENITY_HPP

#include <string>
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

      this->reset();
    }

    return Nothing();
  }

  virtual Try<Nothing> _syncConsume(const std::vector<T> products) = 0;

  // Currently we don't ensure that for in every iteration we fill consumer,
  // so we have to reset counter in every iteration.
  // TODO(bplotka): That would not be needed if we continue pipeline always.
  virtual Try<Nothing> reset() {
    this->products.clear();

    return Nothing();
  }

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
  QOS_CONTROLLER,
  UNDEFINED,
};


class Tag {
 public:
  Tag(const ModuleType& _type, const std::string& _name)
    : type(_type), name(_name) {
    this->fullName = this->init();
  }

  const std::string init() {
    switch (this->type) {
      case RESOURCE_ESTIMATOR:
        this->prefix = "[SerenityEstimator] ";
        this->aim = "Slack estimation";
        break;
      case QOS_CONTROLLER:
        this->prefix = "[SerenityQoS] ";
        this->aim = "QoS assurance";
        break;
      default:
        this->prefix = "[Serenity] ";
        this->aim = "";
        break;
    }

    return this->prefix + this->name + ": ";
  }

  const inline std::string NAME() const {
    return fullName;
  }

  const inline ModuleType TYPE() const {
    return type;
  }

  const inline std::string AIM() const {
    return aim;
  }

 protected:
  const std::string name;
  const ModuleType type;
  std::string fullName;
  std::string prefix;
  std::string aim;
};

#define SERENITY_LOG(severity) LOG(severity) << this->tag.NAME()

}  // namespace serenity
}  // namespace mesos


#endif  // SERENITY_SERENITY_HPP
