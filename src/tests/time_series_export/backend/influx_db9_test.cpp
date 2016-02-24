#include <string>

#include "tests/common/sources/json_source.hpp"
#include "tests/common/mocks/mock_sink.hpp"

#include "time_series_export/backend/influx_db9.hpp"
#include "time_series_export/resource_usage_ts_export.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(InfluxDb9BackendTests, PutMetricWithoutTags) {
  InfluxDb9Backend backend("localhost", "8086", "serenity", "root", "root");

  const double_t VALUE = 48.0;
  TimeSeriesRecord record(Series::CPU_USAGE_SYS, VALUE);

  backend.PutMetric(record);
}

TEST(InfluxDb9BackendTests, PutMetricWithTag) {
  InfluxDb9Backend backend("localhost", "8086", "serenity", "root", "root");

  const double_t VALUE = 66.0;
  const std::string HOSTNAME("localhostname");
  TimeSeriesRecord record(Series::CYCLES, VALUE);
  record.setTag(TsTag::HOSTNAME, HOSTNAME);

  backend.PutMetric(record);
}

TEST(InfluxDb9BackendTests, PutMetricWithMultipleTags) {
  InfluxDb9Backend backend("localhost", "8086", "serenity", "root", "root");

  const double_t VALUE = 88.0;
  const std::string FRAMEWORK_ID("framework_id_tag");
  const std::string EXECUTOR_ID("executor_id_tag");

  TimeSeriesRecord record(Series::INSTRUCTIONS, VALUE);
  record.setTag(TsTag::FRAMEWORK_ID, FRAMEWORK_ID);
  record.setTag(TsTag::EXECUTOR_ID, EXECUTOR_ID);

  backend.PutMetric(record);
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos
