#ifndef SERENITY_EXECUTOR_SET_HPP
#define SERENITY_EXECUTOR_SET_HPP

#include <string>

#include <unordered_set>

#include "mesos/mesos.hpp"

namespace mesos {
namespace serenity {

/**
 * Hasher functor for ExecutorSet
 */
struct ExecutorSetHasher{
  size_t operator()(const ResourceUsage_Executor& that) const {
    if (!that.has_executor_info()) {
      return 0;
    } else {
      std::string hashKey = that.executor_info().executor_id().value() +
                            that.executor_info().framework_id().value();
      std::hash<std::string> hashFunc;
      return hashFunc(hashKey);
    }
  }
};

/**
 * Equals functor for ExecutorSet
 */
struct ExecutorSetEquals {
  bool operator()(const ResourceUsage_Executor& lhs,
                  const ResourceUsage_Executor& rhs) const {
    if (!lhs.has_executor_info() && !rhs.has_executor_info()) {
      return true;
    } else {
      return (lhs.has_executor_info() == rhs.has_executor_info()) &&
             (lhs.executor_info().executor_id().value()   ==
              rhs.executor_info().executor_id().value())  &&
             (lhs.executor_info().framework_id().value()  ==
              rhs.executor_info().framework_id().value());
    }
  }
};

/**
 * Unordered set for storing ResourceUsage_Executor objects.
 * Don't put here objects without executor_info() or the equals and hashcode
 * function won't have any sense,
 */
typedef std::unordered_set<ResourceUsage_Executor,
    ExecutorSetHasher,
    ExecutorSetEquals> ExecutorSet;

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EXECUTOR_SET_HPP
