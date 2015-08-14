#ifndef SERENITY_METRICS_HELPER_HPP
#define SERENITY_METRICS_HELPER_HPP

#include <iomanip>
#include <iostream>
#include <string>
#include <ratio> // NOLINT [build/c++11]

#include "mesos/mesos.hpp"

#include "stout/error.hpp"
#include "stout/try.hpp"

namespace mesos {
namespace serenity {

/**
 * Transforms double timestamp from eg. Resource Usage to string,
 * with certain precision
 * @param _timestamp double timestamp
 * @param _precision std::ratio of precision. Default std::micro
 */
template <typename Ratio = std::milli>
inline std::string DblTimestampToString(
    const double_t _timestamp,
    const Ratio _precision = std::micro()) {
  // count number of places after decimal
  int precision = (std::to_string(_precision.den)).length() - 1;

  std::stringstream timeStream;
  timeStream << std::fixed << std::setprecision(precision)
  << _timestamp;

  std::string timeStr(timeStream.str());
  timeStr.erase(std::remove(
                    timeStr.begin(),
                    timeStr.end(),
                    '.'),
                timeStr.end());

  return timeStr;
}


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

  if (std::isnan(instructionsPerCycle)) {
    return Error("Cannot count IPC usage: 0 cycles.");
  }

  return instructionsPerCycle;
}

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_METRICS_HELPER_HPP
