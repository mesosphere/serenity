#ifndef SERENITY_DROP_FILTER_HPP
#define SERENITY_DROP_FILTER_HPP

#include <memory>
#include <type_traits>

#include <glog/logging.h>

#include "messages/serenity.hpp"

#include "serenity/data_utils.hpp"
#include "serenity/executor_map.hpp"
#include "serenity/executor_set.hpp"
#include "serenity/serenity.hpp"

#include "stout/lambda.hpp"
#include "stout/nothing.hpp"

namespace mesos {
namespace serenity {

/**
 * Sequential change point detection interface.
 * It can receive and process observations sequentially over time.
 */
class ChangePointDetector {
 public:
  ChangePointDetector() {};

  virtual Try<bool> processSample(const double_t& in) = 0;
};


/**
 * Naive implementation of sequential change point detection.
 */
class MeanChangePointDetector {
public:
  MeanChangePointDetector() {};

  virtual Try<bool> processSample(const double_t& in);
};


/**
 * DropFilter is able to check defined value and trigger some contentions
 * on given thresholds.
 */
template <class T>
class DropFilter :
    public Consumer<ResourceUsage>, public Producer<Contentions> {
  static_assert(std::is_base_of<ChangePointDetector, T>::value,
              "T must derive from ChangePointDetector");
 public:
  DropFilter(
      Consumer<Contentions>* _consumer,
      const lambda::function<UsageDataGetterFunction>& _valueGetFunction)
    : Producer<Contentions>(_consumer),
      previousSamples(new ExecutorSet),
      cpDetectors(new ExecutorMap<ChangePointDetector>()),
      valueGetFunction(_valueGetFunction) {}

  ~DropFilter() {}

  Try<Nothing> consume(const ResourceUsage& in);

 protected:
  const lambda::function<UsageDataGetterFunction>& valueGetFunction;
  std::unique_ptr<ExecutorSet> previousSamples;
  std::unique_ptr<ExecutorMap<T>> cpDetectors;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_DROP_FILTER_HPP
