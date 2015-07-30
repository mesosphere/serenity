#ifndef SERENITY_INFLUX_DB8_HPP
#define SERENITY_INFLUX_DB8_HPP

#include <ratio>  // NOLINT [build/c++11]
#include <string>

#include "utils.hpp"

#include "visualisation_backend.hpp"

namespace mesos {
namespace serenity {

class InfluxDb8Backend : public VisualisationBackend {
 public:
  InfluxDb8Backend(std::string _influxDbAddress = DEF_INFLUXDB_ADDRESS,
                   uint32_t _influxDbPort       = DEF_INFLUXDB_PORT,
                   std::string _influxDbName    = DEF_INLUXDB_NAME,
                   std::string _influxDbUser    = DEF_INLUXDB_USER,
                   std::string _influxDbPass    = DEF_INLUXDB_PASS) :
                      influxDbAddess(_influxDbAddress),
                      influxDbPort(_influxDbPort),
                      influxDbName(_influxDbName),
                      influxDbUser(_influxDbUser),
                      influxDbPass(_influxDbPass) {}

  virtual void PutMetric(const VisualisationRecord& _visualisationRecord);


  static constexpr char DEF_INFLUXDB_ADDRESS[] = "localhost";
  static constexpr uint32_t DEF_INFLUXDB_PORT = 8086;

  static constexpr char DEF_INLUXDB_NAME[] = "serenity";
  static constexpr char DEF_INLUXDB_USER[] = "root";
  static constexpr char DEF_INLUXDB_PASS[] = "root";

 protected:
  std::string GetDbUrl() const;
  std::string SerializeRecord(const VisualisationRecord& _visRecord) const;

  const std::string influxDbName;
  const std::string influxDbAddess;
  const uint32_t    influxDbPort;

  const std::string influxDbUser;
  const std::string influxDbPass;

  static constexpr std::micro timePrecision = std::micro();
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_INFLUX_DB8_HPP
