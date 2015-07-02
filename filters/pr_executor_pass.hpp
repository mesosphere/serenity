#ifndef SERENITY_PR_EXECUTOR_PASS_FILTER_HPP
#define SERENITY_PR_EXECUTOR_PASS_FILTER_HPP

#include "serenity/serenity.hpp"

#include "stout/lambda.hpp"
#include "stout/nothing.hpp"

namespace mesos {
namespace serenity {

/**
 * Filter which retain ResourceUsage for production executors only.
 */
class PrExecutorPassFilter :
    public Consumer<ResourceUsage>, public Producer<ResourceUsage> {
 public:
  explicit PrExecutorPassFilter(Consumer<ResourceUsage>* _consumer)
      : Producer<ResourceUsage>(_consumer) {}

  ~PrExecutorPassFilter() {}

  Try<Nothing> consume(const ResourceUsage& in);
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_PR_TASKS_FILTER_HPP
