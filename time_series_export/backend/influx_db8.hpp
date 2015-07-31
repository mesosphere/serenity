#ifndef SERENITY_INFLUX_DB8_HPP
#define SERENITY_INFLUX_DB8_HPP

#include <ratio>  // NOLINT [build/c++11]
#include <string>

#include "serenity/os_utils.hpp"

#include "time_series_backend.hpp"

namespace mesos {
namespace serenity {

class InfluxDb8Backend : public TimeSeriesBackend {
 public:
  InfluxDb8Backend(Option<std::string> _influxDbAddres  = None(),
                   Option<std::string> _influxDbPort    = None(),
                   Option<std::string> _influxDbName    = None(),
                   Option<std::string> _influxDbUser    = None(),
                   Option<std::string> _influxDbPass    = None()) :
      influxDbAddress(EnviromentVariableInitializer(
          _influxDbAddres,
          "INFLUXDB_ADDRESS",
          "localhost")),
      influxDbPort(std::stoi(EnviromentVariableInitializer(
          _influxDbPort,
          "INFLUXDB_PORT",
          "8086"))),
      influxDbName(EnviromentVariableInitializer(
          _influxDbName,
          "INFLUXDB_DB_NAME",
          "serenity")),
      influxDbUser(EnviromentVariableInitializer(
          _influxDbUser,
          "INFLUXDB_USER",
          "root")),
      influxDbPass(EnviromentVariableInitializer(
          _influxDbPass,
          "INFLUXDB_PASSWORD",
          "root")) {}

  virtual void PutMetric(const TimeSeriesRecord& _timeSeriesRecord);

 protected:
  std::string GetDbUrl() const;
  std::string SerializeRecord(const TimeSeriesRecord& _tsRecord) const;

  const std::string influxDbName;
  const std::string influxDbAddress;
  const uint32_t    influxDbPort;

  const std::string influxDbUser;
  const std::string influxDbPass;

  static constexpr std::micro timePrecision = std::micro();
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_INFLUX_DB8_HPP
