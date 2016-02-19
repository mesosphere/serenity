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

    setWindowsSizeAndMaxCheckpoints(
        _config.getItemAndSetDefault<int64_t>(
           SignalDropAnalyzer::WINDOW_SIZE_KEY,
           SignalDropAnalyzer::WINDOW_SIZE_DEFAULT),
        _config.getItemAndSetDefault<int64_t>(
           SignalDropAnalyzer::MAX_CHECKPOINTS_KEY,
           SignalDropAnalyzer::MAX_CHECKPOINTS_DEFAULT));

    setQuroumFraction(_config.getItemAndSetDefault<double_t>(
        SignalDropAnalyzer::QUORUM_FRACTION_KEY,
        SignalDropAnalyzer::QUORUM_FRACTION_DEFAULT));

    setFractionalThreshold(_config.getItemAndSetDefault<double_t>(
        SignalDropAnalyzer::FRACTIONAL_THRESHOLD_KEY,
        SignalDropAnalyzer::FRACTIONAL_THRESHOLD_DEFAULT));

    setNearFraction(_config.getItemAndSetDefault<double_t>(
        SignalDropAnalyzer::NEAR_FRACTION_KEY,
        SignalDropAnalyzer::NEAR_FRACTION_DEFAULT));

    setSeverityFraction(_config.getItemAndSetDefault<double_t>(
        SignalDropAnalyzer::SEVERITY_FRACTION_KEY,
        SignalDropAnalyzer::SEVERITY_FRACTION_DEFAULT));

    this->recalculateParams();
  }

  Result<Detection> _processSample(double_t in);

  virtual Result<Detection> processSample(double_t in);

  virtual Try<Nothing> resetSignalRecovering();

  /**
   * Move each base point to next iterator.
   */
  void shiftBasePoints();

  static const constexpr char* WINDOW_SIZE_KEY = "WINDOW_SIZE";
  static const constexpr char* FRACTIONAL_THRESHOLD_KEY =
    "FRACTIONAL_THRESHOLD";
  static const constexpr char* SEVERITY_FRACTION_KEY = "SEVERITY_FRACTION";
  static const constexpr char* NEAR_FRACTION_KEY = "NEAR_FRACTION";
  static const constexpr char* MAX_CHECKPOINTS_KEY = "MAX_CHECKPOINTS";
  static const constexpr char* QUORUM_FRACTION_KEY = "QUORUM_FRACTION";

protected:
  void recalculateParams();

  //! WindowSize: How far in the past we look.
  //! MaxCheckpoints: Maximum number of checkpoints we will have in our
  //! assurance detector.
  //! Checkpoints are the reference assurance_test(base) points which we refer
  //! to in the past when detecting drop or not.
  //! It needs to be 0 < and < WINDOW_SIZE.
  void setWindowsSizeAndMaxCheckpoints(
      SerenityItem<int64_t> _cfgWindowSize,
      SerenityItem<int64_t> _cfgMaxCheckpoints) {
    cfgWindowSize = _cfgWindowSize.validateValueIsPositive().getValueOrDefault();
    cfgMaxCheckpoints = _cfgMaxCheckpoints
      .validateValueIsPositive()
      .validateValueIsBelow(_cfgWindowSize)
      .getValueOrDefault();
  }

  //! Fraction of checkpoints' votes needed to make a Drop contention.
  void setQuroumFraction(SerenityItem<double_t> item) {
    cfgQuroumFraction = item.validateValueIsPositive().getValueOrDefault();
  }

  //! Defines how much (relatively to base point) value must drop to trigger
  //! contention.
  void setFractionalThreshold(SerenityItem<double_t> item) {
    cfgFractionalThreshold = item.validateValueIsPositive().getValueOrDefault();
  }

  //! You can adjust how big severity is created for a defined drop.
  //! if -1 then unknown severity will be reported.
  void setSeverityFraction(SerenityItem<double_t> item) {
    cfgSeverityFraction = item.getValueOrDefault();
  }

  //! Tolerance fraction of threshold if signal is accepted as returned to
  //! previous state after drop.
  void setNearFraction(SerenityItem<double_t> item) {
    cfgNearFraction = item.validateValueIsPositive().getValueOrDefault();
  }

  std::list<double_t> window;
  std::list<std::list<double_t>::iterator> basePoints;

  // If none then there was no drop.
  Option<double_t> valueBeforeDrop;

  uint64_t dropVotes;
  uint64_t quorumNum;

  // Cfg parameters.
  int64_t cfgWindowSize;
  int64_t cfgMaxCheckpoints;
  double_t cfgQuroumFraction;
  double_t cfgFractionalThreshold;
  double_t cfgSeverityFraction;
  double_t cfgNearFraction;

  // Cfg default values.
  static const constexpr int64_t WINDOW_SIZE_DEFAULT = 10;
  static constexpr double_t FRACTIONAL_THRESHOLD_DEFAULT = 0.3;
  static constexpr double_t SEVERITY_FRACTION_DEFAULT = 2.1;
  static constexpr double_t NEAR_FRACTION_DEFAULT = 0.1;
  static constexpr int64_t MAX_CHECKPOINTS_DEFAULT = 3;
  static constexpr double_t QUORUM_FRACTION_DEFAULT = 0.7;
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_SIGNAL_DROP_ANALYZER_HPP
