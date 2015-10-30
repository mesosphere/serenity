#ifndef SERENITY_CONTENTION_DECIDER_SENIORITY_BASED_HPP
#define SERENITY_CONTENTION_DECIDER_SENIORITY_BASED_HPP

#include "glog/logging.h"

#include "observers/deciders/base.hpp"

#include "serenity/wid.hpp"

namespace mesos {
namespace serenity {

/**
 * Checks contentions and choose executors to kill.
 * Currently it calculates mean contention and based on that estimates how
 * many executors we should kill. Executors are sorted by age.
 */
class SeverityBasedSeniorityDecider : public ContentionDecider {
 public:
  ContentionDeciderFunction decide;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_CONTENTION_DECIDER_SENIORITY_BASED_HPP
