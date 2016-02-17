#ifndef SERENITY_SIGNAL_DROP_ANALYZER_HPP
#define SERENITY_SIGNAL_DROP_ANALYZER_HPP

#include <list>
#include <memory>
#include <string>
#include <iostream>
#include <type_traits>

#include "contention_detectors/signal_analyzers/base.hpp"

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

#define SIGNAL_DROP_ANALYZER_NAME "AssuranceDropAnalyzer"

/**
 * Dynamic implementation of sequential change point detection.
 *
 * There is no warm-up phase - values starts as DEFAULT_START_VALUE.
 * Algorithm steps:
 * - Fetch several basePoints depending on parameters e.g T-1, T-2, T-4, T-8.
 * - Make a voting within all basePoints(checkpoints). Drop will be
 *    detected when dropVotes will be >= Quorum number. Checkpoint will vote
 *    on drop when drop will be higher than value specified in
 *    FRACTIONAL_THRESHOLD.
 * - There are three checkpoint stats in logging:
 *   a) [-] vote on drop.
 *   b) [~] value below checkpoint, but not significant.
 *   c) [+] value is bigger than checkpoint.
 * - When drop appears, start tracking mean value of drop from all basePoints
 *  which voted on drop.
 * - When tracking is active, create contentions until the signal recover or
 *   analyzer is reset externally.
 *
 *  We can use EMA value as input for better results.
 */
class SignalDropAnalyzer : public SignalAnalyzer {
 public:
  explicit SignalDropAnalyzer(
      const Tag& _tag,
      const SerenityConfig& _config)
    : SignalAnalyzer(_tag),
      valueBeforeDrop(None()),
      quorumNum(0) {
    setCfgWindowSize(_config.item<int64_t>(
        detector::WINDOW_SIZE,
        detector::DEFAULT_WINDOW_SIZE));

    setCfgMaxCheckpoints(_config.item<int64_t>(
        detector::MAX_CHECKPOINTS,
        detector::DEFAULT_MAX_CHECKPOINTS));

    setCfgQuroum(_config.item<double_t>(
        detector::QUORUM,
        detector::DEFAULT_QUORUM));

    setCfgFractionalThreshold(_config.item<double_t>(
        detector::FRACTIONAL_THRESHOLD,
        detector::DEFAULT_FRACTIONAL_THRESHOLD));

    setCfgNearFraction(_config.item<double_t>(
        detector::NEAR_FRACTION,
        detector::DEFAULT_NEAR_FRACTION));

    setCfgSeverityFraction(_config.item<double_t>(
        detector::SEVERITY_FRACTION,
        detector::DEFAULT_SEVERITY_FRACTION));

    this->recalculateParams();
  }

  Result<Detection> _processSample(double_t in);

  virtual Result<Detection> processSample(double_t in);

  virtual Try<Nothing> resetSignalRecovering();

  /**
   * Move each base point to next iterator.
   */
  void shiftBasePoints();

  //! int64_t
  //! How far in the past we look.
  void setCfgWindowSize(int64_t cfgWindowSize) {
    SignalDropAnalyzer::cfgWindowSize = cfgWindowSize;
  }

  //! int64_t
  //! Maximum number of checkpoints we will have in our assurance detector.
  //! Checkpoints are the reference assurance_test(base) points which we refer
  //! to in the past when detecting drop or not.
  //! It needs to be 0 < < WINDOW_SIZE
  void setCfgMaxCheckpoints(int64_t cfgMaxCheckpoints) {
    SignalDropAnalyzer::cfgMaxCheckpoints = cfgMaxCheckpoints;
  }

  //! double_t
  //! Fraction of checkpoints' votes that important decision needs to obtain.
  void setCfgQuroum(double_t cfgQuroum) {
    SignalDropAnalyzer::cfgQuroum = cfgQuroum;
  }

  //! double_t
  //! Defines how much (relatively to base point) value must drop to trigger
  //! contention.
  //! Most signal_analyzer will use that.
  void setCfgFractionalThreshold(double_t cfgFractionalThreshold) {
    SignalDropAnalyzer::cfgFractionalThreshold = cfgFractionalThreshold;
  }

  //! double_t
  //! You can adjust how big severity is created for a defined drop.
  //! if -1 then unknown severity will be reported.
  void setCfgSeverityFraction(double_t cfgSeverityFraction) {
    SignalDropAnalyzer::cfgSeverityFraction = cfgSeverityFraction;
  }

  //! double_t
  //! Tolerance fraction of threshold if signal is accepted as returned to
  //! previous state after drop.
  void setCfgNearFraction(double_t cfgNearFraction) {
    SignalDropAnalyzer::cfgNearFraction = cfgNearFraction;
  }

 protected:
  std::list<double_t> window;
  std::list<std::list<double_t>::iterator> basePoints;

  // If none then there was no drop.
  Option<double_t> valueBeforeDrop;

  uint32_t dropVotes;
  uint32_t quorumNum;

  // cfg parameters.
  int64_t cfgWindowSize;
  int64_t cfgMaxCheckpoints;
  double_t cfgQuroum;
  double_t cfgFractionalThreshold;
  double_t cfgSeverityFraction;
  double_t cfgNearFraction;

  /**
   * It is possible to dynamically change analyzer configuration.
   */
  void recalculateParams();
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_SIGNAL_DROP_ANALYZER_HPP
