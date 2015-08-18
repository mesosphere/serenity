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
      influxDbAddress(InitializeField(
          _influxDbAddres,
          "INFLUXDB_ADDRESS",
          "influxdb-monitoring.marathon.mesos")),
      influxDbPort(std::stoi(InitializeField(
          _influxDbPort,
          "INFLUXDB_PORT",
          "8086"))),
      influxDbName(InitializeField(
          _influxDbName,
          "INFLUXDB_DB_NAME",
          "serenity")),
      influxDbUser(InitializeField(
          _influxDbUser,
          "INFLUXDB_USER",
          "root")),
      influxDbPass(InitializeField(
          _influxDbPass,
          "INFLUXDB_PASSWORD",
          "root")) {}

  virtual void PutMetric(const TimeSeriesRecord& _timeSeriesRecord);

 protected:
  std::string GetDbUrl() const;
  std::string SerializeRecord(const TimeSeriesRecord& _tsRecord) const;

/**
 * Initialization helper for constructor.
 * Returns values in order:
 *  - if _constructorValue.isSome - return _constructorValue.get
 *  - if _serviceName.isSome -
 *  - if _enviromenetVariable is true - return enviroment variable
 *  - else return default value
 */
  std::string InitializeField(Option<std::string> _parameterValue,
                              Option<std::string> _envVariableName,
                              std::string _defaultValue);

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
