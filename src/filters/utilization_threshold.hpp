#ifndef SERENITY_UTILIZATION_THRESHOLD_FILTER_HPP
#define SERENITY_UTILIZATION_THRESHOLD_FILTER_HPP

#include <string>
#include <memory>

#include "serenity/default_vars.hpp"
#include "serenity/executor_set.hpp"
#include "serenity/serenity.hpp"

#include "stout/lambda.hpp"
#include "stout/nothing.hpp"

namespace mesos {
namespace serenity {

/**
 * UtilizationThresholdFilter disables oversubscription when node
 * utilization is too high.
 * NOTE: This filter should be the first filter in pipeline. It should have
 * the overview of all tasks running on the node.
 * NOTE: In case of lack of the usage for given executor,
 * filter assumes that executor uses maximum of allowed
 * resource (allocated) and logs warning.
 */
class UtilizationThresholdFilter :
    public Consumer<ResourceUsage>, public Producer<ResourceUsage> {
 public:
  UtilizationThresholdFilter(
      Consumer<ResourceUsage>* _consumer,
      double_t _utilizationThreshold = utilization::DEFAULT_THRESHOLD,
      const Tag& _tag = Tag(UNDEFINED, "utilizationFilter"))
      : tag(_tag), Producer<ResourceUsage>(_consumer),
        utilizationThreshold(_utilizationThreshold),
        previousSamples(new ExecutorSet) {}

  ~UtilizationThresholdFilter() {}

  Try<Nothing> consume(const ResourceUsage& in);

 protected:
  const Tag tag;
  double_t utilizationThreshold;
  std::unique_ptr<ExecutorSet> previousSamples;

  const std::string UTILIZATION_THRESHOLD_FILTER_ERROR = "Filter is not able" \
    " to calculate total cpu usage and cut off oversubscription if needed.";

  const std::string UTILIZATION_THRESHOLD_FILTER_WARNING = "Filter is not" \
    "able to calculate total cpu usage and will base on allocated " \
    " resources to cut off oversubscription if needed.";
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_UTILIZATION_THRESHOLD_FILTER_HPP
