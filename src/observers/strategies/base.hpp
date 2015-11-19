#ifndef SERENITY_STRATEGIES_DECIDER_BASE_HPP
#define SERENITY_STRATEGIES_DECIDER_BASE_HPP

#include <list>
#include <string>

#include "filters/executor_age.hpp"

#include "mesos/slave/oversubscription.hpp"

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "messages/serenity.hpp"

#include "serenity/serenity.hpp"

#include "stout/try.hpp"

namespace mesos {
namespace serenity {

using RevocationStrategyFunction = Try<QoSCorrections>
    (ExecutorAgeFilter* ageFilter,
    const Contentions& currentContentions,
    const ResourceUsage& currentUsage);

/**
 * Convenient base class for contention interpretations.
 * It converts contentions & usage to QoSCorrections.
 *
 * Convenient for debugging and testing different algorithms.
 */
class RevocationStrategy {
 public:
  explicit RevocationStrategy(const Tag& _tag) : tag(_tag) {}

  virtual RevocationStrategyFunction decide = 0;
 protected:
  const Tag tag;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_STRATEGIES_DECIDER_BASE_HPP
