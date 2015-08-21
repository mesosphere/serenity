#ifndef SERENITY_OS_UTILS_HPP
#define SERENITY_OS_UTILS_HPP

#include <unistd.h>
#include <string>

#include "stout/none.hpp"
#include "stout/option.hpp"
#include "stout/try.hpp"


namespace mesos {
namespace serenity {

inline static Option<std::string> GetEnviromentVariable(
    const std::string& _enviromentVariable) {
  if (const char* env = std::getenv(_enviromentVariable.c_str())) {
    return env;
  }
  return None();
}

inline static Try<std::string> GetHostname() {
  constexpr size_t BUF_SIZE = 32;
  char buf[BUF_SIZE];

  int result = gethostname(buf, BUF_SIZE);
  if (result != 0) {
    return Error("Could not get local hostname");
  }
  return std::string(buf);
}

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_OS_UTILS_HPP
