#ifndef SERENITY_DROP_FILTER_HPP
#define SERENITY_DROP_FILTER_HPP

#include <list>
#include <memory>
#include <type_traits>

#include "glog/logging.h"

#include "filters/ema.hpp"

#include "messages/serenity.hpp"

#include "serenity/data_utils.hpp"
#include "serenity/executor_map.hpp"
#include "serenity/executor_set.hpp"
#include "serenity/serenity.hpp"

#include "stout/lambda.hpp"
#include "stout/nothing.hpp"
#include "stout/result.hpp"

namespace mesos {
namespace serenity {

struct ChangePointDetection {
  double_t severity;
};


/**
 * Sequential change point detection interface.
 * It can receive and process observations sequentially over time.
 */
class ChangePointDetector {
 public:
  ChangePointDetector(
      uint64_t _windowSize,
      double_t _absoluteThreshold)
    : windowSize(_windowSize),
      absoluteThreshold(_absoluteThreshold) {
  }

  virtual Result<ChangePointDetection> processSample(double_t in) = 0;

 protected:
  //! Currently we won't use relative threshold.
  double_t absoluteThreshold;
  uint64_t windowSize;
};


/**
 * Naive implementation of sequential change point detection.
 * It checks if the value drops below the absoluteThreshold.
 * We can use EMA value for calculation for better results.
 */
class NaiveChangePointDetector : public ChangePointDetector {
 public:
  NaiveChangePointDetector(
      uint64_t _windowSize, double_t _absoluteThreshold)
  : ChangePointDetector(_windowSize, _absoluteThreshold) {}

  virtual Result<ChangePointDetection> processSample(double_t in);
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
      const lambda::function<usage::GetterFunction>& _valueGetFunction,
      int64_t _windowSize,
      double_t _absoluteThreshold)
    : Producer<Contentions>(_consumer),
      previousSamples(new ExecutorSet),
      cpDetectors(new ExecutorMap<T*>()),
      valueGetFunction(_valueGetFunction),
      windowSize(_windowSize),
      absoluteThreshold(_absoluteThreshold) {}

  ~DropFilter() {}

  Try<Nothing> consume(const ResourceUsage& in) override;

 protected:
  const lambda::function<usage::GetterFunction>& valueGetFunction;
  std::unique_ptr<ExecutorSet> previousSamples;
  std::unique_ptr<ExecutorMap<T*>> cpDetectors;
  int64_t windowSize;
  double_t absoluteThreshold;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_DROP_FILTER_HPP
