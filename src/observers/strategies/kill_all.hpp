#ifndef SERENITY_STRATEGIES_KILL_ALL_HPP
#define SERENITY_STRATEGIES_KILL_ALL_HPP

#include "observers/strategies/base.hpp"

namespace mesos {
namespace serenity {

/**
 * Kills all BE executors given in usage.
 */
class KillAllStrategy : public RevocationStrategy {
 public:
  KillAllStrategy() :
    RevocationStrategy(Tag(QOS_CONTROLLER, "KillAllStrategy")) {}

  RevocationStrategyFunction decide;
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_STRATEGIES_KILL_ALL_HPP
