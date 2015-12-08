#ifndef SERENITY_TESTS_CONFIG_HELPER_HPP
#define SERENITY_TESTS_CONFIG_HELPER_HPP

#include <gtest/gtest.h>

#include "serenity/config.hpp"

namespace mesos {
namespace serenity {
namespace tests {

inline SerenityConfig createAssuranceAnalyzerCfg(
    const uint64_t windowSize,
    const uint64_t maxCheckpoints,
    const double_t fractionalThreshold,
    const double_t severityLvl = detector::DEFAULT_SEVERITY_FRACTION,
    const double_t nearLvl = detector::DEFAULT_NEAR_FRACTION,
    const double_t quorum = detector::DEFAULT_QUORUM) {
  SerenityConfig cfg;
  cfg.set(detector::WINDOW_SIZE, windowSize);
  cfg.set(detector::MAX_CHECKPOINTS, maxCheckpoints);
  cfg.set(detector::FRACTIONAL_THRESHOLD, fractionalThreshold);
  cfg.set(detector::SEVERITY_FRACTION, severityLvl);
  cfg.set(detector::NEAR_FRACTION, nearLvl);
  cfg.set(detector::QUORUM, quorum);

  return cfg;
}

inline SerenityConfig createThresholdDetectorCfg(
  const double_t utilization = detector::DEFAULT_UTILIZATION_THRESHOLD) {
  SerenityConfig cfg;
  cfg.set(detector::THRESHOLD, utilization);

  return cfg;
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_TESTS_CONFIG_HELPER_HPP
