#ifndef SERENITY_OS_UTILS_HPP
#define SERENITY_OS_UTILS_HPP

#include <string>

/**
 * Initialization helper for constructor.
 * Returns values in order:
 *  - if _constructorValue.isSome - return _constructorValue.get
 *  - if _enviromenetVariable is true - return enviroment variable
 *  - else return default value
 */
inline static std::string EnviromentVariableInitializer(
    const Option<std::string>& _constructorValue,
    const char* _enviromentVariable,
    const std::string& _defaultValue) {
  if (_constructorValue.isSome()) {
    return _constructorValue.get();
  } else {
    if (const char* env = std::getenv(_enviromentVariable)) {
      return env;
    } else {
      return _defaultValue;
    }
  }
}

#endif  // SERENITY_OS_UTILS_HPP
