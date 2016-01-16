#include <list>
#include <utility>

#include "bus/event_bus.hpp"

#include "observers/strategies/simple.hpp"

#include "serenity/resource_helper.hpp"

namespace mesos {
namespace serenity {

using std::list;
using std::pair;

Try<QoSCorrections> SimpleStrategy::decide(
    ExecutorAgeFilter* ageFilter,
    const Contentions& currentContentions,
    const ResourceUsage& currentUsage) {

  // TODO(bplotka):!
  return QoSCorrections();
}

}  // namespace serenity
}  // namespace mesos
