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
 * TODO(skonefal): Prepare proper docstring.
 * SignalBasedDetector looks at specific metric of each production executor
 * and emits contention when it's signal drops bellow certain percent of
 * previous value.
 */
class SignalBasedDetector :
    public Consumer<ResourceUsage>,
    public Producer<Contentions> {
 public:
  SignalBasedDetector(
      Consumer<Contentions>* _consumer,
      const lambda::function<usage::GetterFunction>& _getValue,
      SerenityConfig _detectorConf,
      const Tag& _tag = Tag(QOS_CONTROLLER, "SignalBasedDetector"),
      const Contention_Type _contentionType = Contention_Type_IPC)
    : tag(_tag),
      Producer<Contentions>(_consumer),
      detectors(ExecutorMap<std::unique_ptr<SignalAnalyzer>>()),
      getValue(_getValue),
      detectorConf(_detectorConf),
      contentionType(_contentionType) {}

  ~SignalBasedDetector() {}

  Try<Nothing> consume(const ResourceUsage& usage) override;

  static const constexpr char* NAME = "SignalBasedDetector";

 protected:
  const Tag tag;
  const Contention_Type contentionType;
  const lambda::function<usage::GetterFunction> getValue;

  // Detections.
  ExecutorMap<std::unique_ptr<SignalAnalyzer>> detectors;
  SerenityConfig detectorConf;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_SIGNAL_BASED_DETECTOR_FILTER_HPP
