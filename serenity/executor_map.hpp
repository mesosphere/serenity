#ifndef SERENITY_EXECUTOR_MAP_HPP
#define SERENITY_EXECUTOR_MAP_HPP

#include <string>

#include <unordered_map>

namespace mesos {
namespace serenity {

/**
 * Hasher functor for Executor Info
 */
struct ExecutorInfoHasher{
  size_t operator()(const ExecutorInfo& that) const {
    std::string hashKey = that.executor_id().value() +
                          that.framework_id().value();
    std::hash<std::string> hashFunc;
    return hashFunc(hashKey);
  }
};


/**
 * Equals functor for Executor Info
 */
struct ExecutorInfoEquals {
  bool operator()(const ExecutorInfo& lhs,
                  const ExecutorInfo& rhs) const {
    return (lhs.executor_id().value()   ==
            rhs.executor_id().value())  &&
           (lhs.framework_id().value()  ==
            rhs.framework_id().value());
  }
};


/**
 * Unordered map for storing objects where ExecutorInfo is the key.
 */
template <typename Type>
using ExecutorMap = std::unordered_map<ExecutorInfo,
                                       Type,
                                       ExecutorInfoHasher,
                                       ExecutorInfoEquals>;

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EXECUTOR_MAP_HPP
