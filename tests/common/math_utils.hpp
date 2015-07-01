#ifndef SERENITY_MATH_UTILS_HPP
#define SERENITY_MATH_UTILS_HPP

namespace mesos {
namespace serenity {
namespace tests {

/**
 * Default epsilon for near equality for two double_t values in
 * AlmostEq and AlmostZero functions.
 */
constexpr double_t DEFAULT_EPSILON = std::numeric_limits<double_t>::epsilon();

/**
 * Returns near equality between two double_t values.
 * Default epsilon is std::numeric_limits<double_t>::epsilon()
 */
inline bool AlmostEq(
    double_t lhs,
    double_t rhs,
    double_t epsilon = DEFAULT_EPSILON) {
  return abs(lhs - rhs) < epsilon;
}

/**
 * Returns near equality between double_t and 0.0 value.
 * Default epsilon is std::numeric_limits<double_t>::epsilon()
 */
inline bool AlmostZero(
    double_t lhs,
    double_t epsilon = DEFAULT_EPSILON) {
  return AlmostEq(lhs, 0.0, epsilon);
}

}  //  namespace tests
}  //  namespace serenity
}  //  namespace mesos

#endif  // SERENITY_MATH_UTILS_HPP
