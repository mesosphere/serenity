#ifndef SERENITY_CONTENTION_DECIDER_KILL_ALL_HPP
#define SERENITY_CONTENTION_DECIDER_KILL_ALL_HPP

#include "observers/deciders/base.hpp"

namespace mesos {
namespace serenity {

/**
 * Kills all BE executors given in usage.
 */
class KillAllDecider : public ContentionDecider {
 public:
  ContentionDeciderFunction decide;
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_CONTENTION_DECIDER_KILL_ALL_HPP
