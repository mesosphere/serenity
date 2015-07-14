#ifndef SERENITY_VISUALISATION_UTILS_HPP
#define SERENITY_VISUALISATION_UTILS_HPP

#include <iostream>
#include <iomanip>
#include <string>

#include "glog/logging.h"

#include "stout/try.hpp"

namespace mesos {
namespace serenity {

class Precision {
 public:
  static constexpr uint32_t MILI = 3;
  static constexpr uint32_t MIKRO = 6;
  static constexpr uint32_t NANO = 9;
};

inline std::string DblTimestampToString(
    const double_t _timestamp,
    const uint32_t _precision = Precision::MILI) {
  std::stringstream timeStream;
  timeStream << std::fixed << std::setprecision(_precision) << _timestamp;

  std::string timeStr(timeStream.str());
  timeStr.erase(std::remove(timeStr.begin(), timeStr.end(), '.'),
                timeStr.end());

  return timeStr;
}

inline Try<std::string> GetHostName() {
  constexpr uint8_t kBufferLen = 32;
  char hostname[kBufferLen];
  int32_t status = gethostname(hostname, kBufferLen);
  if (status != 0) {
    LOG(ERROR) << "SlackVisualisation: gethostname failed. | errno: " << errno;
    return Error("SlackVisualisation: gethostname failed. | errno: " +
                  std::to_string(errno));
  } else {
    return hostname;
  }
}

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_VISUALISATION_UTILS_HPP
