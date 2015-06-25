#ifndef SERENITY_METRICS_HELPERS_HPP
#define SERENITY_METRICS_HELPERS_HPP

#include <stout/error.hpp>
#include <stout/try.hpp>

#include "mesos/mesos.hpp"

namespace mesos {
namespace serenity {

Try<double_t> CountCpuUsage(const ResourceUsage_Executor& previous,
                            const ResourceUsage_Executor& current) {
  if (!current.has_statistics()                         ||
      !previous.has_statistics()                        ||
      !current.statistics().has_timestamp()             ||
      !current.statistics().has_cpus_user_time_secs()   ||
      !current.statistics().has_cpus_system_time_secs() ||
      !previous.statistics().has_timestamp()            ||
      !previous.statistics().has_cpus_user_time_secs()  ||
      !previous.statistics().has_cpus_system_time_secs()) {
    return Error("Canno count CPU usage, Parameter does not have required "
                     "statistics");
  }

  double_t samplingDuration = current.statistics().timestamp() -
                              previous.statistics().timestamp();
  double_t cpuTimeUsage = (current.statistics().cpus_system_time_secs()  +
                           current.statistics().cpus_user_time_secs())   -
                          (previous.statistics().cpus_system_time_secs() +
                           previous.statistics().cpus_user_time_secs());

  return cpuTimeUsage / samplingDuration;
}

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_METRICS_HELPERS_HPP
