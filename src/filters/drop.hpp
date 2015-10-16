#ifndef SERENITY_DROP_FILTER_HPP
#define SERENITY_DROP_FILTER_HPP

#include <list>
#include <memory>
#include <string>
#include <type_traits>

#include "detectors/detector.hpp"

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
