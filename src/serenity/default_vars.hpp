#ifndef SERENITY_DEFAULT_VARS_HPP
#define SERENITY_DEFAULT_VARS_HPP

#include <cstdint>
#include <cmath>

namespace mesos {
namespace serenity {

namespace ema {

/**
 * Alpha controls how long is the moving average period.
 * The smaller alpha becomes, the longer your moving average is.
 * It becomes smoother, but less reactive to new samples.
 */
constexpr double DEFAULT_ALPHA = 0.2;

}  // namespace ema


namespace changepoint {

constexpr uint64_t DEFAULT_WINDOW_SIZE = 10;
constexpr uint64_t DEFAULT_CONTENTION_COOLDOWN = 10;
constexpr double_t DEFAULT_ABS_THRESHOLD = 0;
constexpr double_t DEFAULT_RELATIVE_THRESHOLD = 20;
constexpr double_t DEFAULT_FRACTIONAL_THRESHOLD = 0.5;
constexpr double_t DEFAULT_SEVERITY_FRACTION = 0.4;
constexpr double_t DEFAULT_NEAR_FRACTION = 0.1;

}  // namespace changepoint


namespace utilization {

constexpr double_t DEFAULT_THRESHOLD = 0.95;

}  // namespace utilization


namespace new_executor {

constexpr uint32_t DEFAULT_THRESHOLD_SEC = 5 * 60;  //!< Five minutes.

}  // namespace new_executor

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_DEFAULT_VARS_HPP
