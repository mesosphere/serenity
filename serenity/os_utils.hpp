#ifndef SERENITY_OS_UTILS_HPP
#define SERENITY_OS_UTILS_HPP

#include <string>

#include "stout/option.hpp"

inline static Option<std::string> GetEnviromentVariable(
    const std::string& _enviromentVariable) {
  if (const char* env = std::getenv(_enviromentVariable.c_str())) {
    return env;
  } else {
    return None();
  }
}

#endif  // SERENITY_OS_UTILS_HPP
