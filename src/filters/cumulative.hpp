#ifndef SERENITY_CUMULATIVE_FILTER_HPP
#define SERENITY_CUMULATIVE_FILTER_HPP

#include <ctime>
#include <list>
#include <memory>
#include <string>

#include "mesos/mesos.hpp"

#include "serenity/serenity.hpp"


namespace mesos {
namespace serenity {

class CumulativeFilter :
    public Consumer<ResourceUsage>, public Producer<ResourceUsage> {
 public:
  CumulativeFilter();

  explicit CumulativeFilter(Consumer<ResourceUsage>* _consumer);

  ~CumulativeFilter();

  Try<Nothing> consume(const ResourceUsage& in);
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_CUMULATIVE_FILTER_HPP
