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

/**
 * SlackResourceObserver observes incoming ResourceUsage
 * and produces Resource with revocable flag set (Slack Resources).
 *
 * Currently it only counts CPU slack
 */
class SlackResourceObserver : public Consumer<ResourceUsage>,
                              public Producer<Resources> {
 public:
  SlackResourceObserver() : previousSamples(new ExecutorSet()) {}

  ~SlackResourceObserver() {}

  Try<Nothing> consume(const ResourceUsage& usage) override;

 protected:
  Result<double_t> CalculateCpuSlack(const ResourceUsage_Executor& prev,
                                     const ResourceUsage_Executor& current)
                                     const;

//  Result<Resource> CombineSlack(
//      const std::vector<Resource>& slackResources) const;

  std::unique_ptr<ExecutorSet> previousSamples;

  /** Don't report slack when it's less than this value */
  static constexpr double_t SLACK_EPSILON = 0.001;

 private:
  SlackResourceObserver(const SlackResourceObserver& other){}
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_SLACK_RESOURCE_HPP
