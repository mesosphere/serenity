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
  SeniorityStrategy() : RevocationStrategy(Tag(QOS_CONTROLLER, NAME)) {
    initialize();
  }

  /**
   * TODO(skonefal): SerenityConfig should have const methods inside.
   *                 Currently, it cannot be passed as const.
   */
  explicit SeniorityStrategy(SerenityConfig _config)
      : RevocationStrategy(Tag(QOS_CONTROLLER, NAME)) {
    initialize();
    if (_config.hasKey(STARTING_SEVERITY_KEY)) {
       severity = _config.getD(STARTING_SEVERITY_KEY);
    }
  }

  Try<QoSCorrections> decide(ExecutorAgeFilter*,
                             const Contentions&,
                             const ResourceUsage&);

  static const constexpr char* STARTING_SEVERITY_KEY = "STARTING_SEVERITY";
  static const constexpr char* NAME = "SeniorityStrategy";

 private:
  void initialize() {
    severity = DEFAULT_SEVERITY;
  }

  static const constexpr double_t DEFAULT_SEVERITY = 0.1;

  double_t severity;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_STRATEGIES_DECIDER_SENIORITY_HPP
