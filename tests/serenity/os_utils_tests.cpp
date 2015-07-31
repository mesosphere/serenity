#include <stdlib.h>
#include <string>

#include "gtest/gtest.h"
#include "mesos/mesos.hpp"

#include "serenity/os_utils.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(EnviromentVariableInitializer, GetEnvVariable) {
  const std::string envName = "SERENITY_TEST_ENV_VAR";
  std::string envVal = "IT_WORKS";

  // set test variable.
  int result = setenv(envName.c_str(), envVal.c_str(), 1);
  ASSERT_EQ(result, 0);

  std::string res = EnviromentVariableInitializer(None(),
                                                  envName.c_str(),
                                                  "err");
  ASSERT_EQ(res, envVal);
}


TEST(EnviromentVariableInitializerTest, GetConstructorVar) {
  const std::string envName = "SERENITY_TEST_ENV_VAR";
  std::string envVal = "bad-envVal";
  std::string constructorVal = "good";

  // set test variable. Initializer should take constructor val.
  int result = setenv(envName.c_str(), envVal.c_str(), 1);
  ASSERT_EQ(result, 0);

  std::string res = EnviromentVariableInitializer(constructorVal,
                                                  envName.c_str(),
                                                  "bad-default");
  ASSERT_EQ(res, constructorVal);
}


TEST(EnviromentVariableInitializerTest, GetDefaultConstructor) {
  const std::string envName = "SERENITY_TEST_ENV_VAR_NON_EXISTANT";
  std::string envVal = "bad-envVal";

  std::string res = EnviromentVariableInitializer(None(),
                                                  envName.c_str(),
                                                  "good-default");
  ASSERT_EQ(res, "good-default");
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

