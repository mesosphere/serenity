#ifndef SERENITY_EXECUTOR_AGE_FILTER_HPP
#define SERENITY_EXECUTOR_AGE_FILTER_HPP

#include <list>
#include <memory>
#include <string>
#include <time.h>


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
  Try<double_t> age(ExecutorInfo exec_id);

  Try<Nothing> ageOrder(std::list<ResourceUsage_Executor>& executors);

 private:
  std::unique_ptr<ExecutorMap<double_t>> started;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EXECUTOR_AGE_FILTER_HPP
