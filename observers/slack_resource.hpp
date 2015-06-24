#ifndef SERENITY_SLACK_RESOURCE_HPP
#define SERENITY_SLACK_RESOURCE_HPP

#include <mesos/mesos.hpp>

#include <stout/result.hpp>

#include <memory>
#include <unordered_set>
#include <string>

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

struct ExecutorSnapshot {

  ExecutorSnapshot(std::string _frameworkId,
                   std::string _executorId,
                   double_t _timestamp,
                   double_t  _cpus_time) :
                      frameworkID(_frameworkId),
                      executorID(_executorId),
                      timestamp(_timestamp),
                      cpuUsageTime(_cpus_time) {};

  size_t countHash() {
    std::string hashKey = this->executorID + this->frameworkID;
    std::hash<std::string> hashFunc;
    return hashFunc(hashKey);
  }

  inline bool operator == (const ExecutorSnapshot &rhs) const {
    return this->executorID == rhs.executorID &&
          this->frameworkID == rhs.frameworkID;
  }

  double_t timestamp;
  std::string frameworkID;
  std::string executorID;
  double_t cpuUsageTime; //<! CPU time for userspace+kernel in [s]
};

struct ExecutorSnapshotHasher{
  size_t operator()(const ExecutorSnapshot& that) const {
    std::string hashKey = that.executorID + that.frameworkID;
    std::hash<std::string> hashFunc;
    return hashFunc(hashKey);
  }
};


/**
 * SlackResourceObserver observers incoming ResourceUsage
 * and produces Resource with revocable flag.
 *
 * Currently it only counts CPU slack
 */
class SlackResourceObserver : public Consumer<ResourceUsage>,
                              public Producer<Resource>
{
public:

  SlackResourceObserver() : previousSamples(
      new std::unordered_set<ExecutorSnapshot, ExecutorSnapshotHasher>()) {};

  ~SlackResourceObserver(){

  };

  virtual Try<Nothing> consume(const ResourceUsage& usage) override;

protected:
  Result<Resource> CalculateSlack(const ExecutorSnapshot& prev,
                                  const ExecutorSnapshot& current,
                                  double_t cpu_allocation) const;

  Result<Resource> CombineSlack(
      const std::vector<Resource>& slackResources) const;

  std::unique_ptr<std::unordered_set<ExecutorSnapshot,
      ExecutorSnapshotHasher>> previousSamples;

  /** Don't report slack when it's less than this value */
  static constexpr double_t SLACK_EPSILON = 0.001;

private:
  SlackResourceObserver(const SlackResourceObserver& other){};

};

} // namespace serenity
} // namespace mesos

#endif //SERENITY_SLACK_RESOURCE_HPP
