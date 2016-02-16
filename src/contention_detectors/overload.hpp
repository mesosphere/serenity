#ifndef SERENITY_OVERLOAD_DETECTOR_HPP
#define SERENITY_OVERLOAD_DETECTOR_HPP

#include <list>
#include <string>

#include "glog/logging.h"

#include "messages/serenity.hpp"

#include "serenity/config.hpp"
#include "serenity/data_utils.hpp"
#include "serenity/serenity.hpp"
#include "serenity/wid.hpp"

#include "stout/lambda.hpp"
#include "stout/nothing.hpp"
#include "stout/option.hpp"

namespace mesos {
namespace serenity {


/**
 * OverloadDetector is able to create contention if utilization is above
 * given thresholds.
 */
class OverloadDetector :
    public Consumer<ResourceUsage>,
    public Producer<Contentions> {
 public:
  OverloadDetector(
      Consumer<Contentions>* _consumer,
      const lambda::function<usage::GetterFunction>& _cpuUsageGetFunction,
      const SerenityConfig& _conf,
      const Tag& _tag = Tag(QOS_CONTROLLER, NAME))
    : tag(_tag),
      cpuUsageGetFunction(_cpuUsageGetFunction),
      Producer<Contentions>(_consumer) {
    configure(_conf);
  }

  ~OverloadDetector() {}

  static const constexpr char* NAME = "OverloadDetector";

 protected:
  void configure(const SerenityConfig& externalConf) {
    SerenityConfig config = SerenityConfig();

    //! double_t
    //! Detector threshold.
    config.set(detector::THRESHOLD, detector::DEFAULT_UTILIZATION_THRESHOLD);

    config.applyConfig(externalConf);

    // cfgUtilizationThreshold = config.getD(detector::THRESHOLD).get();
  }

  void allProductsReady() override;
  bool validate(const ResourceUsage_Executor& inExec);

  const Tag tag;
  const lambda::function<usage::GetterFunction> cpuUsageGetFunction;

  // cfg parameters.
  double_t cfgUtilizationThreshold;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_OVERLOAD_DETECTOR_HPP
