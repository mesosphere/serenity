#ifndef SERENITY_SLACK_RESOURCE_HPP
#define SERENITY_SLACK_RESOURCE_HPP

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "stout/result.hpp"

#include "serenity/executor_set.hpp"
#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

// TODO(bplotka): Make default role configurable from another source
// (not only env)and as a default pass *
inline std::string getDefaultRole() {
  if (const char* env = std::getenv("MESOS_DEFAULT_ROLE")) {
    return env;
  }
  return "*";
}


/**
 * SlackResourceObserver observes incoming ResourceUsage
 * and produces Resource with revocable flag set (Slack Resources).
 *
 * Currently it only counts CPU slack
 */
class SlackResourceObserver : public Consumer<ResourceUsage>,
                              public Producer<Resources> {
 public:
  SlackResourceObserver()
      : previousSamples(new ExecutorSet()), default_role(getDefaultRole()) {}

  explicit SlackResourceObserver(Consumer<Resources>* _consumer)
    : Producer<Resources>(_consumer),
      previousSamples(new ExecutorSet()),
      default_role(getDefaultRole()) {}

  ~SlackResourceObserver() {}

  Try<Nothing> consume(const ResourceUsage& usage) override;

  static constexpr const char* name = "[Serenity] SlackObserver: ";

 protected:
  Result<double_t> CalculateCpuSlack(const ResourceUsage_Executor& prev,
                                     const ResourceUsage_Executor& current)
                                     const;

  std::unique_ptr<ExecutorSet> previousSamples;

  /** Don't report slack when it's less than this value */
  static constexpr double_t SLACK_EPSILON = 0.001;

 private:
  SlackResourceObserver(const SlackResourceObserver& other)
      : default_role(getDefaultRole()) {}
  std::string default_role;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_SLACK_RESOURCE_HPP
