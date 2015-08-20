#ifndef SERENITY_EXECUTOR_AGE_FILTER_HPP
#define SERENITY_EXECUTOR_AGE_FILTER_HPP

#include <time.h>

#include <string>

#include "mesos/mesos.hpp"

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
  Try<double> age(const FrameworkID& frameworkId, const ExecutorID& id);

 private:
  typedef std::map<const std::string, time_t> ExecutorMap;
  typedef std::map<const std::string, ExecutorMap> FrameworkMap;

  FrameworkMap started;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EXECUTOR_AGE_FILTER_HPP
