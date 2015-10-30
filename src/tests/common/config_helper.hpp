#ifndef SERENITY_TESTS_CONFIG_HELPER_HPP
#define SERENITY_TESTS_CONFIG_HELPER_HPP

#include <gtest/gtest.h>

#include "serenity/config.hpp"

namespace mesos {
namespace serenity {
namespace tests {

inline SerenityConfig createAssuranceDetectorCfg(
    const uint64_t windowSize,
    const uint64_t contentionCooldown,
    const double_t fractionalThreshold,
    const double_t severityLvl = detector::DEFAULT_SEVERITY_FRACTION,
    const double_t nearLvl = detector::DEFAULT_NEAR_FRACTION) {
  SerenityConfig cfg;
  cfg.set(detector::WINDOW_SIZE, windowSize);
  cfg.set(detector::CONTENTION_COOLDOWN, contentionCooldown);
  cfg.set(detector::FRACTIONAL_THRESHOLD, fractionalThreshold);
  cfg.set(detector::SEVERITY_FRACTION, severityLvl);
  cfg.set(detector::NEAR_FRACTION, nearLvl);

  return cfg;
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_TESTS_CONFIG_HELPER_HPP
