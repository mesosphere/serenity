#ifndef SERENITY_METRICS_HELPER_HPP
#define SERENITY_METRICS_HELPER_HPP

#include <stout/error.hpp>
#include <stout/try.hpp>

#include "mesos/mesos.hpp"

namespace mesos {
namespace serenity {

inline Try<double_t> CountCpuUsage(const ResourceUsage_Executor& previous,
                            const ResourceUsage_Executor& current) {
  if (!current.has_statistics() ||
      !previous.has_statistics() ||
      !current.statistics().has_timestamp() ||
      !current.statistics().has_cpus_user_time_secs() ||
      !current.statistics().has_cpus_system_time_secs() ||
      !previous.statistics().has_timestamp() ||
      !previous.statistics().has_cpus_user_time_secs() ||
      !previous.statistics().has_cpus_system_time_secs()) {
    return Error("Cannot count CPU usage, Parameter does not have required "
                     "statistics");
  }

  double_t samplingDuration = current.statistics().timestamp() -
                              previous.statistics().timestamp();

  if (samplingDuration == 0)
    return 0;

  double_t cpuTimeUsage = (current.statistics().cpus_system_time_secs() +
                           current.statistics().cpus_user_time_secs()) -
                          (previous.statistics().cpus_system_time_secs() +
                           previous.statistics().cpus_user_time_secs());

  return cpuTimeUsage / samplingDuration;
}

inline Try<double_t> CountIpc(const ResourceUsage_Executor& previous,
                       const ResourceUsage_Executor& current) {
  if (!current.has_statistics() || !previous.has_statistics())
    return Error("Cannot count IPC, Parameter does not have required "
                     "statistics");
  if (!current.statistics().has_perf() ||
      !previous.statistics().has_perf() ||
      !current.statistics().perf().has_timestamp() ||
      !previous.statistics().perf().has_timestamp() ||
      !current.statistics().perf().has_cycles() ||
      !previous.statistics().perf().has_cycles() ||
      !current.statistics().perf().has_instructions() ||
      !previous.statistics().perf().has_instructions()) {
    return Error("Cannot count IPC usage, Parameter does not have required "
                     "perf statistics");
  }

  double_t instructions = current.statistics().perf().instructions() -
                          previous.statistics().perf().instructions();
  double_t cycles = current.statistics().perf().cycles() -
                    previous.statistics().perf().cycles();
  double_t instructionsPerCycle = instructions / cycles;

  return instructionsPerCycle;
}

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_METRICS_HELPER_HPP
