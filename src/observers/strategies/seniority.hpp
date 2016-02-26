#ifndef SERENITY_STRATEGIES_DECIDER_SENIORITY_HPP
#define SERENITY_STRATEGIES_DECIDER_SENIORITY_HPP

#include "glog/logging.h"

#include "observers/strategies/base.hpp"

#include "serenity/config.hpp"
#include "serenity/wid.hpp"

namespace mesos {
namespace serenity {

/**
 * Checks contentions and choose executors to kill.
 * Currently it calculates mean contention and based on that estimates how
 * many executors we should kill. Executors are sorted by age.
 *
 * It also steers the valve filter using EventBus.
 */
class SeniorityStrategy : public RevocationStrategy {
 public:
  /**
   * TODO(skonefal): SerenityConfig should have const methods inside.
   *                 Currently, it cannot be passed as const.
   */
  explicit SeniorityStrategy(const Config& _config)
      : RevocationStrategy(Tag(QOS_CONTROLLER, NAME)) {
    setSeverity(_config.getValue<double_t>(STARTING_SEVERITY_KEY));
  }

  Try<QoSCorrections> decide(ExecutorAgeFilter*,
                             const Contentions&,
                             const ResourceUsage&);

  void setSeverity(Result<double_t> value) {
    severity = ConfigValidator<double_t>(value, STARTING_SEVERITY_KEY)
      .getOrElse(SEVERITY_DEFAULT);
  }

  static const constexpr char* STARTING_SEVERITY_KEY = "STARTING_SEVERITY";
  static const constexpr char* NAME = "SeniorityStrategy";

 private:
  double_t severity;

  static const constexpr double_t SEVERITY_DEFAULT = 0.1;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_STRATEGIES_DECIDER_SENIORITY_HPP
