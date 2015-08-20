#ifndef SERENITY_EXECUTOR_AGE_FILTER_HPP
#define SERENITY_EXECUTOR_AGE_FILTER_HPP

#include <time.h>

#include <string>

#include "mesos/mesos.hpp"

#include "serenity/executor_map.hpp"
#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

class ExecutorAgeFilter :
    public Consumer<ResourceUsage>, public Producer<ResourceUsage> {
 public:
  explicit ExecutorAgeFilter();

  ExecutorAgeFilter(Consumer<ResourceUsage>* _consumer);

  ~ExecutorAgeFilter();

  Try<Nothing> consume(const ResourceUsage& in);

  /**
   * Returns the age of an executor in seconds.
   */
  Try<double> age(const ExecutorInfo& exec_id);

 private:
  std::unique_ptr<ExecutorMap<time_t>> started;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EXECUTOR_AGE_FILTER_HPP
