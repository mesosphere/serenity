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
constexpr bool DEFAULT_ENABLED_VISUALISATION = false;

const constexpr char* ON_EMPTY_CORRECTION_INTERVAL =
  "ON_EMPTY_CORRECTION_INTERVAL";
constexpr double_t DEFAULT_ON_EMPTY_CORRECTION_INTERVAL = 2;


}  // namespace qos_pipeline


namespace ema {
/**
 * Alpha controls how long is the moving average period.
 * The smaller alpha becomes, the longer your moving average is.
 * It becomes smoother, but less reactive to new samples.
 */
const constexpr char* ALPHA = "ALPHA";
constexpr double_t DEFAULT_ALPHA = 0.2;

const constexpr char* ALPHA_CPU = "ALPHA_CPU";
constexpr double_t DEFAULT_ALPHA_CPU = 0.9;
const constexpr char* ALPHA_IPC = "ALPHA_IPC";
constexpr double_t DEFAULT_ALPHA_IPC = 0.9;

}  // namespace ema

namespace detector {
const constexpr char* ANALYZER_TYPE = "ANALYZER_TYPE";
const constexpr char* WINDOW_SIZE = "WINDOW_SIZE";
constexpr int64_t DEFAULT_WINDOW_SIZE = 10;
const constexpr char* FRACTIONAL_THRESHOLD = "FRACTIONAL_THRESHOLD";
constexpr double_t DEFAULT_FRACTIONAL_THRESHOLD = 0.3;
const constexpr char* SEVERITY_FRACTION = "SEVERITY_FRACTION";
constexpr double_t DEFAULT_SEVERITY_FRACTION = 2.1;
const constexpr char* NEAR_FRACTION = "NEAR_FRACTION";
constexpr double_t DEFAULT_NEAR_FRACTION = 0.1;
const constexpr char* MAX_CHECKPOINTS = "MAX_CHECKPOINTS";
constexpr int64_t DEFAULT_MAX_CHECKPOINTS = 3;
const constexpr char* QUORUM = "QUORUM";
constexpr double_t DEFAULT_QUORUM = 0.70;

constexpr double_t DEFAULT_START_VALUE = 0.00001;

const constexpr char* THRESHOLD = "THRESHOLD";
constexpr double_t DEFAULT_UTILIZATION_THRESHOLD = 0.72;
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

namespace too_low_usage {
const constexpr char* MINIMAL_CPU_USAGE = "MINIMAL_CPU_USAGE";
constexpr double_t DEFAULT_MINIMAL_CPU_USAGE = 0.25;  // !< per sec.
}  // namespace too_low_usage

namespace strategy {
const constexpr char* CONTENTION_COOLDOWN = "CONTENTION_COOLDOWN";
constexpr int64_t DEFAULT_CONTENTION_COOLDOWN = 10;
const constexpr char* DEFAULT_CPU_SEVERITY = "DEFAULT_CPU_SEVERITY";
constexpr double_t DEFAULT_DEFAULT_CPU_SEVERITY = 1.0;
static const constexpr char* STARTING_SEVERITY = "STARTING_SEVERITY";
constexpr double_t DEFAULT_STARTING_SEVERITY = 0.1;
}  // namespace strategy

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_DEFAULT_VARS_HPP
