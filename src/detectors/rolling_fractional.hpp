#ifndef SERENITY_ROLLING_FR_DROP_DETECTOR_HPP
#define SERENITY_ROLLING_FR_DROP_DETECTOR_HPP

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
 * Dynamic implementation of sequential change point detection.
 * Algorithm steps:
 * - Warm up phase: wait "windowsSize" iterations.
 * - fetch base point value from (currentIteration - "windowsSize").
 * - Check if new value drops more than fraction of basePoint specified
 *   in fractionalThreshold option.
 *
 *  We can use EMA value as input for better results.
 */
class RollingFractionalDetector : public ChangePointDetector {
public:
  explicit RollingFractionalDetector(const Tag& _tag)
    : ChangePointDetector(_tag) {}

  virtual Result<ChangePointDetection> processSample(double_t in);

protected:
  std::list<double_t> window;
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_ROLLING_FR_DROP_DETECTOR_HPP
