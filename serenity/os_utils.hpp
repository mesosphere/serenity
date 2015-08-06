#ifndef SERENITY_OS_UTILS_HPP
#define SERENITY_OS_UTILS_HPP

#include <string>

#include "stout/none.hpp"
#include "stout/option.hpp"

namespace mesos {
namespace serenity {

inline static Option<std::string> GetEnviromentVariable(
    const std::string& _enviromentVariable) {
  if (const char* env = std::getenv(_enviromentVariable.c_str())) {
    return env;
  }
  return None();
}

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_OS_UTILS_HPP
