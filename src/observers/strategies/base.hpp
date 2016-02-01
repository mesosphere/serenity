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

/**
 * Base class for contention interpretations.
 * It converts contentions & usage to QoSCorrections.
 */
class RevocationStrategy {
 public:
  // TODO(skonefal): Abstract classes should not have tag.
  explicit RevocationStrategy(const Tag& _tag) : tag(_tag) {}

  virtual ~RevocationStrategy() {}

  // TODO(skonefal): Executor Age should be part of Resource Usage.
  virtual Try<QoSCorrections> decide(
      ExecutorAgeFilter* exeutorAge,
      const Contentions& contentions,
      const ResourceUsage& usage) = 0;
 protected:
  const Tag tag;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_STRATEGIES_DECIDER_BASE_HPP
