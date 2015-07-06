#ifndef SERENITY_UTILIZATION_THRESHOLD_FILTER_HPP
#define SERENITY_UTILIZATION_THRESHOLD_FILTER_HPP

#include <string>

#include "serenity/serenity.hpp"
#include "stout/lambda.hpp"
#include "stout/nothing.hpp"

namespace mesos {
namespace serenity {

const double_t DEFAULT_UTILIZATION_THRESHOLD = 0.95;

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
      double_t _utilizationThreshold = DEFAULT_UTILIZATION_THRESHOLD)
      : Producer<ResourceUsage>(_consumer),
        utilizationThreshold(_utilizationThreshold) {}

  ~UtilizationThresholdFilter() {}

  Try<Nothing> consume(const ResourceUsage& in);

 protected:
  double_t utilizationThreshold;
  std::unique_ptr<ExecutorSet> previousSamples;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_UTILIZATION_THRESHOLD_FILTER_HPP
