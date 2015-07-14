#ifndef SERENITY_INFLUX_DB8_HPP
#define SERENITY_INFLUX_DB8_HPP

#include <string>

#include "utils.hpp"

#include "visualisation_backend.hpp"

namespace mesos {
namespace serenity {

constexpr char DEF_INFLUX_DB_ADDRESS[] = "localhost";
constexpr uint32_t DEF_INFLUX_DB_PORT = 8086;

constexpr char DEF_INLUX_DB_NAME[] = "test";

constexpr char DEF_INLUX_DB_USER[] = "admin";
constexpr char DEF_INLUX_DB_PASS[] = "admin";

constexpr uint8_t DEF_INFLUX_DB_TIME_PREC = Precision::MILI;

class InfluxDb8Backend : public IVisualisationBackend {
 public:
  InfluxDb8Backend(std::string _influxDbAddress = DEF_INFLUX_DB_ADDRESS,
                   uint32_t _influxDbPort       = DEF_INFLUX_DB_PORT,
                   std::string _influxDbName    = DEF_INLUX_DB_NAME,
                   std::string _influxDbUser    = DEF_INLUX_DB_USER,
                   std::string _influxDbPass    = DEF_INLUX_DB_PASS,
                   uint8_t _precision           = DEF_INFLUX_DB_TIME_PREC) :
                      influxDbAddess(_influxDbAddress),
                      influxDbPort(_influxDbPort),
                      influxDbName(_influxDbName),
                      influxDbUser(_influxDbUser),
                      influxDbPass(_influxDbPass),
                      timePrecision(_precision) {}

  virtual void PutMetric(const VisualisationRecord& _visualisationRecord);

 protected:
  std::string GetDbUrl() const;
  std::string SerializeRecord(const VisualisationRecord& _visRecord) const;

  const std::string influxDbName;
  const std::string influxDbAddess;
  const uint32_t    influxDbPort;

  const std::string influxDbUser;
  const std::string influxDbPass;

  const uint8_t timePrecision;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_INFLUX_DB8_HPP
