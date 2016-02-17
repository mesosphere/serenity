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
    // Parse config values.
    setCfgUtilizationThreshold(
      _conf.item<double_t>(detector::THRESHOLD,
                           detector::DEFAULT_UTILIZATION_THRESHOLD));
  }

  ~OverloadDetector() {}

  static const constexpr char* NAME = "OverloadDetector";

  void setCfgUtilizationThreshold(double_t cfgUtilizationThreshold) {
    OverloadDetector::cfgUtilizationThreshold = cfgUtilizationThreshold;
  }

 protected:
  void allProductsReady() override;
  bool hasRequiredFields(const ResourceUsage_Executor& inExec);

  const Tag tag;
  const lambda::function<usage::GetterFunction> cpuUsageGetFunction;

  // cfg parameters.
  double_t cfgUtilizationThreshold;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_OVERLOAD_DETECTOR_HPP
