#ifndef SERENITY_TIME_SERIES_RECORD_HPP_HPP
#define SERENITY_TIME_SERIES_RECORD_HPP_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include "boost/variant.hpp"

#include "stout/nothing.hpp"
#include "stout/option.hpp"
#include "stout/try.hpp"

namespace mesos {
namespace serenity {

/**
 * Enum of data series names
 */
enum class Series : uint8_t {
  SLACK_RESOURCES,
  CPU_USAGE_SYS,
  CPU_USAGE_USR,
  CPU_USAGE_SUM,
  CPU_ALLOC,
  CYCLES,
  INSTRUCTIONS,
  CPI,
  IPC,
  CACHE_MISSES,
  STARTED_TASKS,
  RUNNING_TASKS,
  FINISHED_TASKS,
  REVOKED_TASKS,
  FAILED_TASKS
};


static std::string SeriesString(Series series) {
  switch (series) {
    case Series::SLACK_RESOURCES: return "slack_resources";
    case Series::CPU_USAGE_SYS:   return "cpu_usage_sys";
    case Series::CPU_USAGE_USR:   return "cpu_usage_usr";
    case Series::CPU_USAGE_SUM:   return "cpu_usage_sum";
    case Series::CPU_ALLOC:       return "cpu_allocation";
    case Series::CYCLES:          return "cycles";
    case Series::INSTRUCTIONS:    return "instructions";
    case Series::CPI:             return "cpi";
    case Series::IPC:             return "ipc";
    case Series::CACHE_MISSES:    return "cache_misses";
    case Series::STARTED_TASKS:   return "started_tasks";
    case Series::RUNNING_TASKS:   return "running_tasks";
    case Series::FINISHED_TASKS:  return "finished_tasks";
    case Series::REVOKED_TASKS:   return "revoked_tasks";
    case Series::FAILED_TASKS:    return "failed_tasks";
  }
}


/**
 * Enum of commonly used column names for storing serenity data
 */
enum class TsTag : uint8_t {
  TASK_ID,
  EXECUTOR_ID,
  FRAMEWORK_ID,
  HOSTNAME,
  TASK_NAME,
  AGENT_ID,
  TAG,
};

static std::string TagString(TsTag tag) {
  switch (tag) {
    case TsTag::TASK_ID:      return "taskId";
    case TsTag::EXECUTOR_ID:  return "executorId";
    case TsTag::FRAMEWORK_ID: return "frameworkId";
    case TsTag::HOSTNAME:     return "node";
    case TsTag::TASK_NAME:    return "task_name";
    case TsTag::AGENT_ID:     return "agentId";
    case TsTag::TAG:          return "tag";
  }
}

/**
 * Record to be stored in time series backend
 */
class TimeSeriesRecord {
  using Value = boost::variant<uint64_t, int64_t, double_t, std::string>;

 public:
  TimeSeriesRecord(const Series _series,
                   Value _value) :
      tags(std::unordered_map<std::string, std::string>()),
      value(_value),
      seriesName(SeriesString(_series)) {}

  const std::string getSeriesName() const {
    return seriesName;
  }

  const std::unordered_map<std::string, std::string> getTags() const {
    return tags;
  }

  Value getValue() const {
    return value;
  }

  /**
   * Set column value in database
   */
  template <typename T>
  void inline setTag(const std::string& tag, const T& val) {
    tags[tag] = val;
  }


  /**
   * Set column value in database
   * Key is one of commonly used tags
   */
  template <typename T>
  void inline setTag(const TsTag tag, const T& val) {
    tags[TagString(tag)] = val;
  }

 private:
  /**
   * String with tags that will be added to database
   * It's string->string, and not tag->string for purpose
   * of adding custom tags.
   */
  std::unordered_map<std::string, std::string> tags;
  Value value;

  const std::string      seriesName;  //!< Series name in backend.
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_TIME_SERIES_RECORD_HPP_HPP
