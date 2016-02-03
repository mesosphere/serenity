#ifndef SERENITY_SERENITY_HPP
#define SERENITY_SERENITY_HPP

#include <string>
#include <vector>

#include "glog/logging.h"

#include "stout/nothing.hpp"
#include "stout/try.hpp"

namespace mesos {
namespace serenity {

class BaseFilter {
  template <typename T>
  friend class Consumer;
  template <typename T>
  friend class Producer;
 public:
  /**
   * Function invoked when filter consumes all products in iteration.
   * When filter consumes more than one product, this function should
   * be overriden.
   */
  virtual Try<Nothing> allProductsReady() {
    return Nothing();
  }

  /**
   * Function invoked when filter consumes all the products in iteration
   * after 'consumeAllProducts'. Developer can override it to clean
   * data after computations.
   */
  virtual void cleanup() {
    return;
  }

 protected:
  BaseFilter() : producerCount(0),
                 productsConsumedCounter(0) {}

  virtual ~BaseFilter() {}

 private:
  void registerProducer() {
    producerCount += 1;
  }

  void newProductForConsumption() {
    productsConsumedCounter += 1;
  }

  bool isAllProductsConsumed() {
    return productsConsumedCounter == producerCount;
  }

  void cleanAfterIteration() {
    cleanup();
    productsConsumedCounter = 0;
  }

  uint32_t producerCount;
  uint32_t productsConsumedCounter;
};


template<typename T>
class Consumer : virtual public BaseFilter {
  template <typename F>
  friend class Producer;
 protected:
  virtual Try<Nothing> consume(const T& in) = 0;

  Consumer() : BaseFilter() {}

  virtual ~Consumer() {}

 private:
  Try<Nothing> _consume(const T& in) {
    newProductForConsumption();
    // let derived class consume the product
    Try<Nothing> result = consume(in);
    if (isAllProductsConsumed() && !result.isError()) {
      result = allProductsReady();
    }
    cleanAfterIteration();
    return result;
  }
};


template<typename T>
class Producer : virtual public BaseFilter {
 public:
  Try<Nothing> addConsumer(Consumer<T>* consumer) {
    if (consumer != nullptr) {
      consumers.push_back(consumer);
      consumer->registerProducer();
    } else {
      LOG(ERROR) << "Consumer must not be null.";
    }
    return Nothing();
  }

protected:
  Producer() {}

  explicit Producer(Consumer<T>* consumer) {
    addConsumer(consumer);
  }

  virtual ~Producer() {}

  Try<Nothing> produce(T out) {
    for (auto consumer : consumers) {
      Try<Nothing> ret = consumer->_consume(out);

      // Stop the pipeline in case of error.
      if (ret.isError()) {
        LOG(ERROR) << ret.error() << " | Error during produce";
        return ret;
      }
    }
    return Nothing();
  }

private:
  std::vector<Consumer<T>*> consumers;
};


/**
 * This consumer requires that all producents will run consume on
 * this instance. (Even during error flow).
 */
template<typename T>
class SyncConsumer : public Consumer<T> {
 public:
  explicit SyncConsumer(uint64_t _producentsToWaitFor) : Consumer<T>(),
    producentsToWaitFor(_producentsToWaitFor) {
    CHECK_ERR(_producentsToWaitFor > 0);
  }

  virtual ~SyncConsumer() {}

  Try<Nothing> consume(const T& in) {
    this->products.push_back(in);

    if (consumedAllNeededProducts()) {
      // Run virtual function which should be implemented in derived
      // class.
      this->syncConsume(this->products);

      // Reset need to be done explicitly.
      // this->reset();
    }

    return Nothing();
  }

  virtual Try<Nothing> syncConsume(const std::vector<T> products) = 0;

  // Currently we don't ensure that for in every iteration we fill consumer,
  // so we have to reset counter in every iteration.
  // TODO(bplotka): That would not be needed if we continue pipeline always.
  virtual Try<Nothing> reset() {
    this->products.clear();

    return Nothing();
  }

  // You can enforce pipeline to continue the flow even if only some
  // of the producents produced the needed object.
  Try<Nothing> ensure() {
    // We do syncConsume only when it wasn't done earlier.
    if (!consumedAllNeededProducts()) {
      this->syncConsume(this->products);
    }

    return this->reset();
  }

 protected:
  uint64_t producentsToWaitFor;
  std::vector<T> products;

  bool consumedAllNeededProducts() {
    return (this->products.size() == this->producentsToWaitFor);
  }
};


enum ModuleType {
  RESOURCE_ESTIMATOR,
  QOS_CONTROLLER,
  UNDEFINED,
};

#define SERENITY_LOG(severity) LOG(severity) << tag.NAME()

// TODO(skonefal): Tag class should overload operator <<
class Tag {
 public:
  Tag(const ModuleType& _type, const std::string& _name)
      : type(_type) {
    this->name = getPrefix() + ": ";
  }

  explicit Tag(const std::string& _name)
    : type(UNDEFINED) {
    std::string prefix = getPrefix();
    this->name = prefix + _name + ": ";
  }

  const inline std::string NAME() const {
    return name;
  }

  const inline ModuleType TYPE() const {
    return type;
  }

 private:
  const std::string getPrefix() {
    std::string prefix;
    switch (this->type) {
      case RESOURCE_ESTIMATOR:
        prefix = "[SerenityEstimator] ";;
        break;
      case QOS_CONTROLLER:
        prefix = "[SerenityQoS] ";
        break;
      default:
        prefix = "[Serenity] ";
        break;
    }

    return prefix;
  }

  const ModuleType type;
  std::string name;
};

}  // namespace serenity
}  // namespace mesos


#endif  // SERENITY_SERENITY_HPP
