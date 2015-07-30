#include <string>
#include <sstream>

#include "curl/curl.h"
#include "curl_easy.h"  // NOLINT(build/include)

#include "influx_db8.hpp"

namespace mesos {
namespace serenity {

using curl::curl_easy;

void InfluxDb8Backend::PutMetric(const VisualisationRecord& _visRecord) {
  std::string url = this->GetDbUrl();
  std::string content = this->SerializeRecord(_visRecord);

  curl_easy easy;

  easy.add(curl_pair<CURLoption, std::string>(CURLOPT_URL, url));
  easy.add(curl_pair<CURLoption, int64_t>(CURLOPT_POST, 1));
  easy.add(curl_pair<CURLoption, int64_t>(CURLOPT_POSTFIELDSIZE,
                                          content.size()));
  easy.add(curl_pair<CURLoption, std::string>(CURLOPT_POSTFIELDS, content));
  easy.perform();

  return;
}


std::string InfluxDb8Backend::GetDbUrl() const {
  constexpr uint32_t kBufferLen = 256;
  char buffer[kBufferLen];
  snprintf(buffer, kBufferLen, "http://%s:%d/db/%s/series?u=%s&p=%s",
                 this->influxDbAddess.c_str(),
                 this->influxDbPort,
                 this->influxDbName.c_str(),
                 this->influxDbUser.c_str(),
                 this->influxDbPass.c_str());

  return std::string(buffer);
}



std::string InfluxDb8Backend::SerializeRecord(
    const VisualisationRecord& _visRecord) const {
  constexpr char SEP = ',';
  std::string result;
  std::string series = "\"name\": \"" + _visRecord.getSeriesName() + "\"";
  std::stringstream columnsStream; columnsStream << "\"columns\": [";
  std::stringstream pointsStream;  pointsStream  << "\"points\": [ [";

  // Add time.
  std::string timestamp = DblTimestampToString(
      _visRecord.getTimestamp().get(),
      this->timePrecision);

  columnsStream << "\"time\"" << SEP;
  pointsStream << timestamp << SEP;

  const auto& tags = _visRecord.getTags();
  for (const auto& tag : tags) {
    std::string key = tag.first;
    auto value = tag.second;

    // Adding key to json
    columnsStream << "\"" << key << "\"" << SEP;

    if (VisualisationRecord::isVariantString(value)) {
      pointsStream << "\"" << value << "\"" << SEP;
    } else {
      pointsStream << value << SEP;
    }
  }

  std::string columns = columnsStream.str();
  columns.pop_back();
  columns.append("]");

  std::string points = pointsStream.str();
  points.pop_back();
  points.append("] ]");

  result = "[{" + series + SEP + columns + SEP + points + "}]";

  return result;
}

}  // namespace serenity
}  // namespace mesos