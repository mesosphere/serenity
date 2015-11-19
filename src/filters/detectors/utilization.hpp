#ifndef SERENITY_HIGH_UTILIZATION_DETECTOR_HPP
#define SERENITY_HIGH_UTILIZATION_DETECTOR_HPP

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

#define UTILIZATION_DETECTOR_NAME "UtilizationDetector"

class UtilizationDetectorConfig : public SerenityConfig {
 public:
  UtilizationDetectorConfig() {}

  explicit UtilizationDetectorConfig(const SerenityConfig& customCfg) {
    this->initDefaults();
    this->applyConfig(customCfg);
  }

  void initDefaults() {
    this->fields[detector::DETECTOR_TYPE] = UTILIZATION_DETECTOR_NAME;
    //! double_t
    //! CPU utilization threshold.
    this->fields[detector::UTILIZATION_THRESHOLD] =
      detector::DEFAULT_UTILIZATION_THRESHOLD;
  }
};


/**
 * In case of high utilization trigger a contention.
 * Utilization threshold value is configurable.
 */
class UtilizationDetector : public BaseDetector {
 public:
  explicit UtilizationDetector(
      const Tag& _tag,
      const SerenityConfig& _config)
      : BaseDetector(_tag) {
    SerenityConfig config = UtilizationDetectorConfig(_config);
    this->cfgUtilizationThreshold =
      config.getD(detector::UTILIZATION_THRESHOLD);
  }

  virtual Result<Detection> processSample(double_t in);

  virtual Try<Nothing> reset();

 protected:
  std::list<double_t> window;

  // cfg parameters.
  double_t cfgUtilizationThreshold;
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_HIGH_UTILIZATION_DETECTOR_HPP
