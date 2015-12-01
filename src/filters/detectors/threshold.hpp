#ifndef SERENITY_THRESHOLD_DETECTOR_HPP
#define SERENITY_THRESHOLD_DETECTOR_HPP

#include <list>
#include <memory>
#include <string>
#include <iostream>
#include <type_traits>

#include "filters/detectors/base.hpp"
#include "filters/ema.hpp"

#include "glog/logging.h"

#include "messages/serenity.hpp"

#include "serenity/config.hpp"
#include "serenity/default_vars.hpp"
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

#define THRESHOLD_DETECTOR_NAME "ThresholdDetector"

class ThresholdDetectorConfig : public SerenityConfig {
 public:
  ThresholdDetectorConfig() {}

  explicit ThresholdDetectorConfig(const SerenityConfig& customCfg) {
    this->initDefaults();
    this->applyConfig(customCfg);
  }

  void initDefaults() {
    this->fields[detector::DETECTOR_TYPE] = THRESHOLD_DETECTOR_NAME;
    //! double_t
    //! Configurable threshold.
    this->fields[detector::THRESHOLD] =
      detector::DEFAULT_UTILIZATION_THRESHOLD;
  }
};


/**
 * In case of high value trigger a contention.
 * Threshold value is configurable.
 */
class ThresholdDetector : public BaseDetector {
 public:
  explicit ThresholdDetector(
      const Tag& _tag,
      const SerenityConfig& _config)
      : BaseDetector(_tag) {
    SerenityConfig config = ThresholdDetectorConfig(_config);
    this->cfgThreshold =
      config.getD(detector::THRESHOLD);
  }

  virtual Result<Detection> processSample(double_t in);

  virtual Try<Nothing> reset();

 protected:
  std::list<double_t> window;

  // cfg parameters.
  double_t cfgThreshold;
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_THRESHOLD_DETECTOR_HPP
