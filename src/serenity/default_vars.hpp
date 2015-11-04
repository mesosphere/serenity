#ifndef SERENITY_DEFAULT_VARS_HPP
#define SERENITY_DEFAULT_VARS_HPP

#include <cstdint>
#include <cmath>

namespace mesos {
namespace serenity {

namespace qos_pipeline {
const constexpr char* VALVE_OPENED = "VALVE_OPENED";
constexpr bool DEFAULT_VALVE_OPENED = true;
const constexpr char* ENABLED_VISUALISATION = "ENABLED_VISUALISATION";
constexpr bool DEFAULT_ENABLED_VISUALISATION = true;
}  // namespace qos_pipeline


namespace ema {
/**
 * Alpha controls how long is the moving average period.
 * The smaller alpha becomes, the longer your moving average is.
 * It becomes smoother, but less reactive to new samples.
 */
const constexpr char* ALPHA = "ALPHA";
constexpr double_t DEFAULT_ALPHA = 0.2;
}  // namespace ema


namespace detector {
const constexpr char* DETECTOR_TYPE = "DETECTOR_TYPE";
const constexpr char* WINDOW_SIZE = "WINDOW_SIZE";
constexpr uint64_t DEFAULT_WINDOW_SIZE = 10;
const constexpr char* FRACTIONAL_THRESHOLD = "FRACTIONAL_THRESHOLD";
constexpr double_t DEFAULT_FRACTIONAL_THRESHOLD = 0.5;
const constexpr char* SEVERITY_FRACTION = "SEVERITY_FRACTION";
constexpr double_t DEFAULT_SEVERITY_FRACTION = 0.4;
const constexpr char* NEAR_FRACTION = "NEAR_FRACTION";
constexpr double_t DEFAULT_NEAR_FRACTION = 0.1;
const constexpr char* CHECKPOINTS = "CHECKPOINTS";
constexpr uint64_t DEFAULT_CHECKPOINTS = 3;
const constexpr char* QUORUM = "QUORUM";
constexpr uint64_t DEFAULT_QUORUM = 3;
}  // namespace detector

namespace slack_observer {
constexpr double_t DEFAULT_MAX_OVERSUBSCRIPTION_FRACTION = 0.8;
}  // namespace slack_observer

namespace utilization {
constexpr double_t DEFAULT_THRESHOLD = 0.95;
}  // namespace utilization


namespace new_executor {
constexpr uint32_t DEFAULT_THRESHOLD_SEC = 5 * 60;  // !< Five minutes.
}  // namespace new_executor

namespace decider {
const constexpr char* CONTENTION_COOLDOWN = "CONTENTION_COOLDOWN";
constexpr uint64_t DEFAULT_CONTENTION_COOLDOWN = 10;
}  // namespace decider

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_DEFAULT_VARS_HPP
