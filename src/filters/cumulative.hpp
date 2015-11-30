#ifndef SERENITY_CUMULATIVE_FILTER_HPP
#define SERENITY_CUMULATIVE_FILTER_HPP

#include <ctime>
#include <list>
#include <memory>
#include <string>

#include "mesos/mesos.hpp"

#include "serenity/serenity.hpp"
#include "serenity/executor_set.hpp"

namespace mesos {
namespace serenity {

class CumulativeFilter :
    public Consumer<ResourceUsage>, public Producer<ResourceUsage> {
 public:
  explicit CumulativeFilter(
     Consumer<ResourceUsage>* _consumer,
     const Tag& _tag = Tag(UNDEFINED, "CumulativeFilter"))
     : Producer<ResourceUsage>(_consumer),
       previousSamples(new ExecutorSet()),
       tag(_tag) {}

  ~CumulativeFilter();

  Try<Nothing> consume(const ResourceUsage& in);

 protected:
  const Tag tag;
  std::unique_ptr<ExecutorSet> previousSamples;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_CUMULATIVE_FILTER_HPP
