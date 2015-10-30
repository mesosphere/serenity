#ifndef SERENITY_ASSURANCE_FR_DROP_DETECTOR_HPP
#define SERENITY_ASSURANCE_FR_DROP_DETECTOR_HPP

#include <list>
#include <memory>
#include <string>
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

class AssuranceDetectorConfig : public SerenityConfig {
 public:
  AssuranceDetectorConfig() {}

  explicit AssuranceDetectorConfig(const SerenityConfig& customCfg) {
    this->initDefaults();
    this->applyConfig(customCfg);
  }

  void initDefaults() {
    this->fields[detector::DETECTOR_TYPE] = "AssuranceDetector";
    //! How far in the past we look.
    this->fields[detector::WINDOW_SIZE] =
      detector::DEFAULT_WINDOW_SIZE;

    //! Defines how much (relatively to base point) value must drop to trigger
    //! contention.
    //! Most detectors will use that.
    this->fields[detector::FRACTIONAL_THRESHOLD] =
      detector::DEFAULT_FRACTIONAL_THRESHOLD;

    //! You can adjust how big severity is created for  a defined drop.
    this->fields[detector::SEVERITY_FRACTION] =
      detector::DEFAULT_SEVERITY_FRACTION;

    //! Tolerance fraction of threshold if signal is accepted as returned to
    //! previous state after drop.
    this->fields[detector::NEAR_FRACTION] =
      detector::DEFAULT_NEAR_FRACTION;
  }
};


/**
 * Dynamic implementation of sequential change point detection.
 * Algorithm steps:
 * - Warm up phase: wait "windowsSize" iterations.
 * - fetch base point value from (currentIteration - "windowsSize").
 * - Check if new value drops more than fraction of basePoint specified
 *   in fractionalThreshold option.
 * - When drop appears, check if the value will return after corrections.
 *  If not, trigger more contentions.
 *
 *  We can use EMA value as input for better results.
 */
class AssuranceDetector : public BaseDetector {
 public:
  explicit AssuranceDetector(
      const Tag& _tag,
      const SerenityConfig& _config)
    : BaseDetector(_tag, AssuranceDetectorConfig(_config)),
      referencePoint(None()),
      referencePointCounter(0),
      lastSeverity(0) {}

  virtual Result<Detection> processSample(double_t in);

  static const constexpr char* NAME = "AssuranceDetector";
 protected:
  std::list<double_t> window;
  Option<double_t> referencePoint;
  uint64_t referencePointCounter;
  double_t lastSeverity;
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_ASSURANCE_FR_DROP_DETECTOR_HPP
