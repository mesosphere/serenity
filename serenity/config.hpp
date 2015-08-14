#ifndef SERENITY_CONFIG_HPP
#define SERENITY_CONFIG_HPP

#include "serenity/default_vars.hpp"

namespace mesos {
namespace serenity {


/**
 * State for every Change Point Detector. It enables user to
 * configure change Point Detectors.
 * Detectors are also able to adjust these values dynamically.
 */
struct ChangePointDetectionState {
  static ChangePointDetectionState createForNaiveDetector(
      uint64_t _contentionCooldown,
      double_t _absoluteThreshold) {
    ChangePointDetectionState state;
    state.contentionCooldown = _contentionCooldown;
    state.absoluteThreshold = _absoluteThreshold;

    return state;
  }

  static ChangePointDetectionState createForRollingDetector(
      uint64_t _windowSize,
      uint64_t _contentionCooldown,
      double_t _relativeThreshold) {
    ChangePointDetectionState state;
    state.windowSize = _windowSize;
    state.contentionCooldown = _contentionCooldown;
    state.relativeThreshold = _relativeThreshold;

    return state;
  }

  uint64_t windowSize = changepoint::DEFAULT_WINDOW_SIZE;
  //! How many iterations detector will wait with creating another
  //! contention.
  uint64_t contentionCooldown = changepoint::DEFAULT_CONTENTION_COOLDOWN;

  //! NaiveChangePointDetector bases its filtering on absolute value.
  //! Below that value detector will trigger contention.
  double_t absoluteThreshold = changepoint::DEFAULT_ABS_THRESHOLD;

  //! Defines how much value must drop to trigger contention.
  //! Most detectors will use that.
  double_t relativeThreshold = changepoint::DEFAULT_RELATIVE_THRESHOLD;
};


struct QoSPipelineConf {
  QoSPipelineConf() {}

  explicit QoSPipelineConf(
      ChangePointDetectionState _cpdState,
      double_t _emaAlpha,
      double_t _utilizationThreshold,
      bool _visualisation,
      bool _valveOpened)
    : cpdState(_cpdState),
      emaAlpha(_emaAlpha),
      utilizationThreshold(_utilizationThreshold),
      visualisation(_visualisation),
      valveOpened(_valveOpened) {}

  ChangePointDetectionState cpdState;

  double_t emaAlpha = ema::DEFAULT_ALPHA;

  double_t utilizationThreshold = utilization::DEFAULT_THRESHOLD;

  bool visualisation = true;

  bool valveOpened = false;
};

}  // namespace serenity
}  // namespace mesos


#endif  // SERENITY_CONFIG_HPP
