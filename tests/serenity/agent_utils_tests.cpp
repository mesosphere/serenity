#include "gtest/gtest.h"
#include "mesos/mesos.hpp"

#include "serenity/agent_utils.hpp"

namespace mesos {
namespace serenity {
namespace tests {

TEST(AgentUtilsTest, GetHostname) {
  auto result = AgentInfo::GetHostName();
  ASSERT_TRUE(result.isSome());
}

TEST(AgentUtilsTest, GetAgentId) {
  auto result = AgentInfo::GetAgentId();
  ASSERT_TRUE(result.isSome());
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

