#include <stdlib.h>
#include <string>

#include "gtest/gtest.h"

#include "stout/try.hpp"

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

  Option<std::string> res = GetEnviromentVariable(envName.c_str());
  ASSERT_TRUE(res.isSome());
  ASSERT_EQ(res.get(), envVal);
}


TEST(EnviromentVariableInitializer, GetUnexistantEnvVariable) {
  const std::string envName = "SERENITY_TEST_ENV_VAR_NON_EXISTENT";

  Option<std::string> res = GetEnviromentVariable(envName.c_str());
  ASSERT_TRUE(res.isNone());
}


TEST(GetHostname, GetHostname) {
  Try<std::string> hostname = GetHostname();
  ASSERT_TRUE(hostname.isSome());
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

