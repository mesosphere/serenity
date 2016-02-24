#ifndef SERENITY_TIME_SERIES_BACKEND_HPP
#define SERENITY_TIME_SERIES_BACKEND_HPP

#include <vector>

#include "time_series_export/backend/time_series_record.hpp"

namespace mesos {
namespace serenity {

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
