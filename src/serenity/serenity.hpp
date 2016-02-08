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
 protected:
  /**
   * Function invoked when filter consumes all products in iteration.
   * When filter consumes more than one product, this function should
   * be overriden.
   *
   * TODO(skonefal): After consume deprecation, this should be abstract.
   */
  virtual void allProductsReady() {}

  BaseFilter() : consumablesPerIteration(0),
                 consumablesInCurrentIterationCount(0),
                 productionsPerIteration(0),
                 productionsInCurrentIterationCount(0) {}

  virtual ~BaseFilter() {}

 private:
  void registerProductForConsumption() {
    consumablesPerIteration += 1;
  }

  void registerProducer() {
    productionsPerIteration += 1;
  }

  void newProductConsumed() {
    consumablesInCurrentIterationCount += 1;
    if (isAllProductsConsumed()) {
      allProductsConsumed();
    }
  }

  void productProduced() {
    productionsInCurrentIterationCount += 1;
  }

  bool isAllProductsConsumed() {
    return consumablesInCurrentIterationCount == consumablesPerIteration;
  }

  bool isAllProductsProduced() {
    return productionsInCurrentIterationCount == productionsPerIteration;
  }

  bool notAllProductsProduced() {
    return !isAllProductsProduced();
  }

  void allProductsConsumed() {
    // Invoke virtual allProductsReady in derived class.
    allProductsReady();

    // If allProductsReady didn't produced all products - log error.
    if (notAllProductsProduced()) {
      // TODO(skonefal): Make '<<' virutal, so we could log component name.
      LOG(ERROR) << "Component has produced "
                 << productionsInCurrentIterationCount << " products. "
                 <<  "It should produce " << productionsPerIteration;
    }

    cleanAfterIteration();
  }

  void cleanAfterIteration() {
    consumablesInCurrentIterationCount = 0;
    productionsInCurrentIterationCount = 0;
  }

  uint32_t consumablesPerIteration;  //!< Number of expected consumables
  uint32_t consumablesInCurrentIterationCount;  //!< Consumables in Iteration

  uint32_t productionsPerIteration;  //!< Nubmer of expected productions
  uint32_t productionsInCurrentIterationCount;  //!< Productions in iteration
};


template<typename T>
class Consumer : virtual public BaseFilter {
  template <typename F>
  friend class Producer;
 protected:
  Consumer() : BaseFilter(),
               productsPerIteration(0),
               cleanConsumables(false) {}

  virtual ~Consumer() {}

  // TODO(skonefal): consume method should be deprecated
  // and allProductsReady should be only exposed to users.
  virtual Try<Nothing> consume(const T& in) {
    return Nothing();
  }

  /**
   * Returns vector of products that came to Consumer.
   * TODO(skonefal): Should we only return iterator to consumables?
   */
  const std::vector<T>& getConsumables() const {
    return consumables;
  }

  /**
   * Returns first product that came to consumer, or None if not available.
   */
  const Option<T> getConsumable() const {
    if (consumables.size() > 0) {
      return consumables[0];
    } else {
      return None();
    }
  }

 private:
  void registerProductForConsumption() {
    productsPerIteration += 1;
    BaseFilter::registerProductForConsumption();
  }

  // TODO(skonefal): Rename to 'consume' after current 'consume' deprecation.
  void _consume(const T& in) {
    if (cleanConsumables) {
      consumables.clear();
    }

    consumables.push_back(in);
    // Let derived class consume the product.
    // TODO(skonefal): We should deprecate consume method.
    consume(in);

    // Consumer has it's own track of consumed products.
    if (isAllProductsConsumed()) {
      cleanConsumables = true;
    }

    // Inform base filter that new product is consumed.
    newProductConsumed();
  }

  void addProductToConsumables(const T &in) {
    if (cleanConsumables) {
      consumables.clear();
    }
  }

  bool isAllProductsConsumed() {
    return consumables.size() == productsPerIteration;
  }

  uint32_t productsPerIteration;  //!< Number of products we expect.
  bool cleanConsumables;

  std::vector<T> consumables;
};


template<typename T>
class Producer : virtual public BaseFilter {
 public:
  void addConsumer(Consumer<T>* consumer) {
    if (consumer != nullptr) {
      consumers.push_back(consumer);
      consumer->registerProductForConsumption();
    } else {
      LOG(ERROR) << "Consumer must not be null.";
    }
  }

 protected:
  Producer() {
    intialize();
  }

  explicit Producer(Consumer<T>* consumer) {
    intialize();
    addConsumer(consumer);
  }

  virtual ~Producer() {}

  Try<Nothing> produce(T out) {
    for (auto consumer : consumers) {
      consumer->_consume(out);
    }
    BaseFilter::productProduced();
    return Nothing();
  }

 private:
  void intialize() {
    BaseFilter::registerProducer();
  }

  std::vector<Consumer<T>*> consumers;
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
