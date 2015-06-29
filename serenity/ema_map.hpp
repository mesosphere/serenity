#ifndef SERENITY_EMA_MAP_HPP
#define SERENITY_EMA_MAP_HPP

#include <string>

#include <unordered_map>

#include "filters/ema.hpp"

#include "mesos/mesos.hpp"


namespace mesos {
namespace serenity {

/**
 * Hasher functor for EmaMap
 */
struct EmaMapHasher{
  size_t operator()(const ExecutorInfo& that) const {
    std::string hashKey = that.executor_id().value() +
                          that.framework_id().value();
    std::hash<std::string> hashFunc;
    return hashFunc(hashKey);
  }
};


/**
 * Equals functor for EmaMap
 */
struct EmaMapEquals {
  bool operator()(const ExecutorInfo& lhs,
                  const ExecutorInfo& rhs) const {
    return (lhs.executor_id().value()   ==
            rhs.executor_id().value())  &&
           (lhs.framework_id().value()  ==
            rhs.framework_id().value());
  }
};


/**
 * Unordered map for storing ExponentialMovingAverage objects.
 */
typedef std::unordered_map<ExecutorInfo,
    ExponentialMovingAverage,
    EmaMapHasher,
    EmaMapEquals> EmaMap;

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EMA_MAP_HPP
