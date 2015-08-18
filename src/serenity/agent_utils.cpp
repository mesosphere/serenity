#include <string>

#include "agent_utils.hpp"

namespace mesos {
namespace serenity {

std::mutex AgentInfo::connectionMutex;
Option<std::string> AgentInfo::hostname = None();
Option<std::string> AgentInfo::agentId = None();

}  // namespace serenity
}  // namespace mesos
