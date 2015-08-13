#ifndef SERENITY_DROP_FILTER_HPP
#define SERENITY_DROP_FILTER_HPP

#include <list>
#include <memory>
#include <string>
#include <type_traits>

#include "glog/logging.h"

#include "filters/ema.hpp"

#include "messages/serenity.hpp"

#include "serenity/config.hpp"
#include "serenity/data_utils.hpp"
#include "serenity/executor_map.hpp"
#include "serenity/executor_set.hpp"
#include "serenity/serenity.hpp"

#include "stout/lambda.hpp"
#include "stout/nothing.hpp"
#include "stout/option.hpp"
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
  ChangePointDetector() {}

  virtual Try<Nothing> configure(ChangePointDetectionState cpdState) {
    this->state = cpdState;

    return Nothing();
  }

  virtual Result<ChangePointDetection> processSample(double_t in) = 0;

 protected:
  ChangePointDetectionState state;
  uint64_t contentionCooldownCounter = 0;
};


/**
 * Naive implementation of sequential change point detection.
 * It checks if the value drops below the absoluteThreshold.
 * NOTE(bplotka): It is usable only when we are sure what specific value is
 * needed for specific executors to not starve.
 * Should NOT be used in real env.
 *
 * We can use EMA value as input for better results.
 */
class NaiveChangePointDetector : public ChangePointDetector {
 public:
  NaiveChangePointDetector() {}

  virtual Result<ChangePointDetection> processSample(double_t in);
};


/**
 * Dynamic implementation of sequential change point detection.
 * Algorithm steps:
 * - Warm up phase: wait "windowsSize" iterations.
 * - fetch base point value from (currentIteration - "windowsSize").
 * - Check if new value drops below the (base point - relativeThreshold).
 *
 *  We should use EMA value as input for better results.
 */
class RollingChangePointDetector : public ChangePointDetector {
 public:
  RollingChangePointDetector() {}

  virtual Result<ChangePointDetection> processSample(double_t in);

 protected:
  std::list<double_t> window;
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
      ChangePointDetectionState _changePointDetectionState,
      const Tag& _tag = Tag(QOS_CONTROLLER, "dropFilter"))
    : tag(_tag),
      Producer<Contentions>(_consumer),
      previousSamples(new ExecutorSet),
      cpDetectors(new ExecutorMap<T*>()),
      valueGetFunction(_valueGetFunction),
      changePointDetectionState(_changePointDetectionState) {}

  ~DropFilter() {}

  Try<Nothing> consume(const ResourceUsage& in) override;

 protected:
  const Tag tag;
  const lambda::function<usage::GetterFunction> valueGetFunction;
  std::unique_ptr<ExecutorSet> previousSamples;
  std::unique_ptr<ExecutorMap<T*>> cpDetectors;
  ChangePointDetectionState changePointDetectionState;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_DROP_FILTER_HPP
