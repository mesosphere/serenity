#ifndef SERENITY_NAIVE_DROP_DETECTOR_HPP
#define SERENITY_NAIVE_DROP_DETECTOR_HPP

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
 * Naive implementation of sequential change point detection.
 * It checks if the value drops below the absoluteThreshold.
 * NOTE(bplotka): It is usable only when we are sure what specific value is
 * needed for specific executors to not starve.
 * Should NOT be used in real env.
 *
 * We can use EMA value as input for better results.
 */
class NaiveChangePointDetector : public ChangePointDetector {
 public:
  explicit NaiveChangePointDetector(const Tag& _tag)
    : ChangePointDetector(_tag) {}

  virtual Result<ChangePointDetection> processSample(double_t in);
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_NAIVE_DROP_DETECTOR_HPP
