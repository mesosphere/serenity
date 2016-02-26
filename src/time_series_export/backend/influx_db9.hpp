#ifndef SERENITY_INFLUX_DB9_HPP
#define SERENITY_INFLUX_DB9_HPP

#include <ratio>  // NOLINT [build/c++11]
#include <string>

#include "curl_easy.h"  // NOLINT(build/include)

#include "serenity/os_utils.hpp"

#include "time_series_backend.hpp"

namespace mesos {
namespace serenity {

class InfluxDb9Backend : public TimeSeriesBackend {
 public:
  InfluxDb9Backend(Option<std::string> _influxDbAddres = None(),
                   Option<std::string> _influxDbPort = None(),
                   Option<std::string> _influxDbDatabaseName = None(),
                   Option<std::string> _influxDbUser = None(),
                   Option<std::string> _influxDbPass = None()) :
      influxDbAddress(initializeField(
          _influxDbAddres,
          "INFLUXDB_ADDRESS",
          "influxdb-monitoring.marathon.mesos")),
      influxDbPort(std::stoi(initializeField(
          _influxDbPort,
          "INFLUXDB_PORT",
          "8086"))),
      influxDbDatabaseName(initializeField(
          _influxDbDatabaseName,
          "INFLUXDB_DB_NAME",
          "serenity")),
      influxDbUser(initializeField(
          _influxDbUser,
          "INFLUXDB_USER",
          "root")),
      influxDbPass(initializeField(
          _influxDbPass,
          "INFLUXDB_PASSWORD",
          "root")) {}

  virtual void PutMetric(const TimeSeriesRecord& _timeSeriesRecord);

 protected:
  const curl::curl_easy prepareRequest(const TimeSeriesRecord& _tsRecord) const;
  const std::string getDbUrl() const;
  const std::string getUserAndPassword() const;
  const std::string serializeRecord(const TimeSeriesRecord& _tsRecord) const;

  /**
   * Initialization helper for constructor.
   * Returns values in order:
   *  - if _constructorValue.isSome - return _constructorValue.get
   *  - if _serviceName.isSome -
   *  - if _enviromenetVariable is true - return enviroment variable
   *  - else return default value
   */
  const std::string initializeField(Option <std::string> _parameterValue,
                                    Option <std::string> _envVariableName,
                                    std::string _defaultValue);

  const std::string influxDbDatabaseName;
  const std::string influxDbAddress;
  const uint32_t    influxDbPort;

  const std::string influxDbUser;
  const std::string influxDbPass;

  static constexpr auto timePrecision = std::nano();
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_INFLUX_DB9_HPP
