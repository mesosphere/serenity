#ifndef SERENITY_TIME_SERIES_BACKEND_HPP
#define SERENITY_TIME_SERIES_BACKEND_HPP

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
  CPU_ALLOC,
  CYCLES,
  INSTRUCTIONS,
  CACHE_MISSES
};

static std::string SeriesString(Series series) {
  switch (series) {
    case Series::SLACK_RESOURCES: return "SlackResources";
    case Series::CPU_USAGE_SYS:   return "CpuUsageSys";
    case Series::CPU_USAGE_USR:   return "CpuUsageUsr";
    case Series::CPU_ALLOC:       return "CpuAllocation";
    case Series::CYCLES:          return "Cycles";
    case Series::INSTRUCTIONS:    return "Instructions";
    case Series::CACHE_MISSES:    return "CacheMisses";
  }
}


/**
 * Enum of commonly used column names for storing serenity data
 */
enum class Tag : uint8_t {
  EXECUTOR_ID,
  FRAMEWORK_ID,
  HOSTNAME,
  AGENT_ID,
  TAG,
  VALUE,
};

static std::string TagString(Tag tag) {
  switch (tag) {
    case Tag::EXECUTOR_ID:  return "ExecutorId";
    case Tag::FRAMEWORK_ID: return "FrameworkId";
    case Tag::HOSTNAME:     return "Node";
    case Tag::AGENT_ID:     return "AgentId";
    case Tag::TAG:          return "Tag";
    case Tag::VALUE:        return "Value";
  }
}


/**
* Variant type for storing multiple types of data that will be stored in
* time series backend.
* This alias must resemble VariantType enum.
*/
using Variant = boost::variant<uint64_t, int64_t, double_t, std::string>;


/**
 * Record to be stored in time series backend
 */
class TimeSeriesRecord {
 public:
  TimeSeriesRecord(const Series _series,
                   const Variant _variant = 0.0,
                   const Option<double_t> _timestamp = None()) :
      seriesName(SeriesString(_series)),
      timestamp(_timestamp),
      tags(std::unordered_map<std::string, Variant>()) {
    setTag(Tag::VALUE, _variant);
  }

/**
 * Variant types inside tags map.
 * Used for preparing json.
 * Must resemble Variant alias.
 */
  enum class VariantType : uint8_t{
    UINT64,
    INT64,
    DOUBLE,
    STRING,
  };

  static bool isVariantString(Variant _variant) {
    return _variant.which() == static_cast<uint8_t>(VariantType::STRING);
  }

  Option<double_t> getTimestamp() const {
    return this->timestamp;
  }


  std::string getSeriesName() const {
    return this->seriesName;
  }


  const std::unordered_map<std::string, Variant>& getTags() const {
    return this->tags;
  }


  /**
   * Set column value in database
   * Key is std::string
   * Value accepts <int32_t, double_t, std::string>
   */
  template <typename T>
  void inline setTag(const std::string& tag, const T& val) {
    tags[tag] = val;
  }


  /**
   * Set column value in database
   * Key is one of commonly used tags
   * Value accepts <int32_t, double_t, std::string>
   */
  template <typename T>
  void inline setTag(const Tag tag, const T& val) {
    tags[TagString(tag)] = val;
  }

 protected:
  /**
   * String with tags that will be added to database
   * It's string->string, and not tag->string for purpose
   * of adding custom tags.
   */
  std::unordered_map<std::string, Variant> tags;

  const std::string      seriesName;  //!< Series name in backend.
  const Option<double_t> timestamp;
};


/**
 * Time Series backend interface.
 */
class TimeSeriesBackend {
 public:
  virtual void PutMetric(const TimeSeriesRecord& _timeSeriesRecord) = 0;

  virtual void PutMetric(const std::vector<TimeSeriesRecord>& _recordList){
    for (const auto& record : _recordList) {
      this->PutMetric(record);
    }
  }
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_TIME_SERIES_BACKEND_HPP
