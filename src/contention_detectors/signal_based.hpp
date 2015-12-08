#ifndef SERENITY_SIGNAL_BASED_DETECTOR_FILTER_HPP
#define SERENITY_SIGNAL_BASED_DETECTOR_FILTER_HPP

#include <list>
#include <memory>
#include <string>
#include <utility>

#include "glog/logging.h"

#include "contention_detectors/signal_analyzers/drop.hpp"
#include "contention_detectors/signal_analyzers/base.hpp"

#include "messages/serenity.hpp"

#include "serenity/config.hpp"
#include "serenity/data_utils.hpp"
#include "serenity/executor_map.hpp"
#include "serenity/executor_set.hpp"
#include "serenity/serenity.hpp"
#include "serenity/resource_helper.hpp"
#include "serenity/wid.hpp"

#include "stout/lambda.hpp"
#include "stout/nothing.hpp"
#include "stout/option.hpp"
#include "stout/result.hpp"

namespace mesos {
namespace serenity {

/**
 * SignalBasedDetector is able to check defined value and trigger some contentions
 * on given thresholds.
 */
class SignalBasedDetector :
    public Consumer<ResourceUsage>,
    public Producer<Contentions> {
 public:
  SignalBasedDetector(
      Consumer<Contentions>* _consumer,
      const lambda::function<usage::GetterFunction>& _valueGetFunction,
      SerenityConfig _detectorConf,
      const Tag& _tag = Tag(QOS_CONTROLLER, "SignalBasedDetector"))
    : tag(_tag),
      Producer<Contentions>(_consumer),
      detectors(new ExecutorMap<std::unique_ptr<SignalAnalyzer>>()),
      valueGetFunction(_valueGetFunction),
      detectorConf(_detectorConf) {}

  ~SignalBasedDetector() {}

  Try<Nothing> consume(const ResourceUsage& in) override;

  Try<Nothing> _detect(const DividedResourceUsage& in);

  static const constexpr char* NAME = "SignalBasedDetector";

 protected:
  const Tag tag;
  const lambda::function<usage::GetterFunction> valueGetFunction;

  // Detections.
  std::unique_ptr<ExecutorMap<std::unique_ptr<SignalAnalyzer>>> detectors;
  SerenityConfig detectorConf;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_SIGNAL_BASED_DETECTOR_FILTER_HPP
