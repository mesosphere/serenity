#ifndef SERENITY_DETECTOR_FILTER_HPP
#define SERENITY_DETECTOR_FILTER_HPP

#include <list>
#include <memory>
#include <string>
#include <utility>

#include "glog/logging.h"

#include "filters/detectors/assurance.hpp"
#include "filters/detectors/base.hpp"
#include "filters/ema.hpp"

#include "messages/serenity.hpp"

#include "serenity/config.hpp"
#include "serenity/data_utils.hpp"
#include "serenity/executor_map.hpp"
#include "serenity/executor_set.hpp"
#include "serenity/serenity.hpp"
#include "serenity/wid.hpp"

#include "stout/lambda.hpp"
#include "stout/nothing.hpp"
#include "stout/option.hpp"
#include "stout/result.hpp"

namespace mesos {
namespace serenity {


/**
 * DetectorFilter is able to check defined value and trigger some contentions
 * on given thresholds.
 */
class DetectorFilter :
    public Consumer<ResourceUsage>, public Producer<Contentions> {
 public:
  DetectorFilter(
      Consumer<Contentions>* _consumer,
      const lambda::function<usage::GetterFunction>& _valueGetFunction,
      SerenityConfig _detectorConf,
      const Tag& _tag = Tag(QOS_CONTROLLER, "detectorFilter"))
    : tag(_tag),
      Producer<Contentions>(_consumer),
      previousSamples(new ExecutorSet),
      detectors(new ExecutorMap<std::shared_ptr<BaseDetector>>()),
      valueGetFunction(_valueGetFunction),
      detectorConf(_detectorConf) {}

  ~DetectorFilter() {}

  Try<Nothing> consume(const ResourceUsage& in) override;

  static const constexpr char* NAME = "Detector";

 protected:
  const Tag tag;
  const lambda::function<usage::GetterFunction> valueGetFunction;
  std::unique_ptr<ExecutorSet> previousSamples;

  // Detections.
  std::unique_ptr<ExecutorMap<std::shared_ptr<BaseDetector>>> detectors;
  SerenityConfig detectorConf;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_DETECTOR_FILTER_HPP
