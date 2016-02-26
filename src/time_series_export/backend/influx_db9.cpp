#include <string>
#include <sstream>

#include "3rdparty/lib/curlcpp/include/curl_easy.h"

#include "glog/logging.h"

#include "serenity/metrics_helper.hpp"

#include "time_series_export/backend/influx_db9.hpp"

namespace mesos {
namespace serenity {

using curl::curl_easy;

void InfluxDb9Backend::PutMetric(const TimeSeriesRecord& _tsRecord) {
  curl_easy easy = prepareRequest(_tsRecord);

  try {
    easy.perform();
  }
  catch (curl_easy_exception error) {
    LOG(ERROR) << "InfluxDB9Backend: Error while inserting metrics, "
               << error.what();
  }
}

const curl_easy InfluxDb9Backend::prepareRequest(
    const TimeSeriesRecord& _tsRecord) const {
  curl_easy easy;

  std::string url = getDbUrl();
  std::string userpwd = getUserAndPassword();
  std::string content = serializeRecord(_tsRecord);

  easy.add(curl_pair<CURLoption, std::string>(CURLOPT_URL, url));

  easy.add(curl_pair<CURLoption, uint64_t>(
      CURLOPT_HTTPAUTH, CURLAUTH_BASIC));
  easy.add(curl_pair<CURLoption, std::string>(CURLOPT_USERPWD, userpwd));

  easy.add(curl_pair<CURLoption, int64_t>(CURLOPT_POST, 1));
  easy.add(curl_pair<CURLoption, int64_t>(CURLOPT_POSTFIELDSIZE,
                                          content.size()));
  easy.add(curl_pair<CURLoption, std::string>(CURLOPT_POSTFIELDS, content));

  return easy;
}

/**
 * Returns URL to database.
 * i.e. http://localhost:8086/write?db=mydb
 */
const std::string InfluxDb9Backend::getDbUrl() const {
  constexpr uint32_t kBufferLen = 256;
  char buffer[kBufferLen];
  snprintf(buffer, kBufferLen, "http://%s:%d/write?db=%s",
           this->influxDbAddress.c_str(),
           this->influxDbPort,
           this->influxDbDatabaseName.c_str());

  return std::string(buffer);
}

const std::string InfluxDb9Backend::getUserAndPassword() const {
  std::stringstream stringstream;
  stringstream << influxDbUser << ":" << influxDbPass;
  return stringstream.str();
}

/**
 * Serializes record to InfluxDB9 format
 * measurement,tag=val,tkey2=tval2 fkey=fval,fkey2=fval2 1234567890000000000
 * <measurement>,<tags> <values <timestamp>
 */
const std::string InfluxDb9Backend::serializeRecord(
    const TimeSeriesRecord& _tsRecord) const {
  constexpr char SPACE_SEP = ' ';
  constexpr char COMMA_SEP = ',';
  const constexpr char* VALUE = "value";
  std::stringstream record;

  record << _tsRecord.getSeriesName();

  for (const auto& tag : _tsRecord.getTags()) {
    record << COMMA_SEP << tag.first << "=" << tag.second;
  }

  record << SPACE_SEP << VALUE << "=" << _tsRecord.getValue();

  return record.str();
}

const std::string InfluxDb9Backend::initializeField(
    Option<std::string> _parameterValue,
    Option<std::string> _envVariableName,
    std::string _defaultValue) {
  if (_parameterValue.isSome()) {
    return _parameterValue.get();
  }
  if (_envVariableName.isSome()) {
    Option<std::string> result = GetEnviromentVariable(
        _envVariableName.get());
    if (result.isSome()) {
      return result.get();
    }
  }
  return _defaultValue;
}

}  // namespace serenity
}  // namespace mesos
