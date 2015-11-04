#ifndef SERENITY_ASSURANCE_FR_DROP_DETECTOR_HPP
#define SERENITY_ASSURANCE_FR_DROP_DETECTOR_HPP

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

    //! Number of checkpoints we will have in our assurance detector.
    //! Checkpoints are the reference (base) points which we refer to in the
    //! past when detecting drop or not. It needs to be 0 < WINDOW_SIZE
    this->fields[detector::CHECKPOINTS] =
      detector::DEFAULT_CHECKPOINTS;

    //! Number of checkpoints' votes that important decision needs to obtain.
    this->fields[detector::QUORUM] =
      detector::DEFAULT_QUORUM;
  }
};


/**
 * Dynamic implementation of sequential change point detection.
 *
 * There is no warm-up phase - values starts as 0.
 * Algorithm steps: TODO:
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
      valueBeforeDrop(None()) {
    // Validation phase.

    if (this->cfg.getU64(detector::QUORUM) >
        this->cfg.getU64(detector::CHECKPOINTS)) {
      SERENITY_LOG(WARNING) << detector::QUORUM << "param cannot be less "
                            << "than " << detector::CHECKPOINTS;
      this->cfg.set(detector::QUORUM, this->cfg.getU64(detector::CHECKPOINTS));
    }

    // TODO(bplotka): Apply different ways to achieve points.
    // Eg. T-n, T-2n, T-4n, T-8n..
    // Currently we base on const checkPointInterval.
    if (this->cfg.getU64(detector::CHECKPOINTS) == 0) {
      SERENITY_LOG(WARNING) << detector::CHECKPOINTS << "param cannot be zero.";
      checkPointInterval = 1;
    } else {
      checkPointInterval =
        this->cfg.getU64(detector::WINDOW_SIZE)/
        this->cfg.getU64(detector::CHECKPOINTS);

      if (checkPointInterval == 0) {
        SERENITY_LOG(WARNING) << detector::WINDOW_SIZE << " parameter "
        << "is required to be higher than " << detector::CHECKPOINTS
        << "param. Assigning: " << detector::CHECKPOINTS << " = "
        << detector::WINDOW_SIZE;
        checkPointInterval = 1;
      }
    }

    // Init window with zeros and choose base points.
    for (int i = 1; i <= this->cfg.getU64(detector::WINDOW_SIZE); i++) {
      this->window.push_back(0.000001);
      if (checkPointInterval == 1 || i % checkPointInterval == 1) {
        if (basePoints.size() < this->cfg.getU64(detector::CHECKPOINTS)) {
          basePoints.push_back(--this->window.end());
        }
      }
    }
  }

  Result<Detection> _processSample(double_t in);

  virtual Result<Detection> processSample(double_t in);

  virtual Try<Nothing> reset();

  /**
   * Move each base point to next iterator.
   */
  void shiftBasePoints();

  /**
   * Contention Factory.
   */
  Detection createContention(double_t severity);

  static const constexpr char* NAME = "AssuranceDetector";

 protected:
  int64_t checkPointInterval = 0;
  std::list<double_t> window;
  std::list<std::list<double_t>::iterator> basePoints;

  // If none then there was no drop.
  Option<double_t> valueBeforeDrop;

  int32_t dropVotes;
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_ASSURANCE_FR_DROP_DETECTOR_HPP
