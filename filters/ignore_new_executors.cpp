#include "glog/logging.h"

#include "filters/ignore_new_executors.hpp"

#include "stout/try.hpp"
#include "stout/nothing.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> IgnoreNewExecutorsFilter::consume(const ResourceUsage &usage) {
  std::unique_ptr<ExecutorMap<time_t>> newExecutorTimestamps =
    std::unique_ptr<ExecutorMap<time_t>>(new ExecutorMap<time_t>());
  ResourceUsage newUsage;

  time_t timeNow = this->GetTime(nullptr);
  auto resultPair = std::make_pair(executorTimestamps->begin() , true);
  for (const auto& executor : usage.executors()) {
    ExecutorInfo executorInfo = executor.executor_info();

    // Find executor or add it if non-existent.
    const auto& prevExecutorEntry =
        this->executorTimestamps->find(executorInfo);
    if (prevExecutorEntry == this->executorTimestamps->end()) {
      resultPair = newExecutorTimestamps->insert(
          std::make_pair(executor.executor_info(),
                         executor.statistics().timestamp()));
    } else {
      resultPair = newExecutorTimestamps->insert(std::make_pair(
          prevExecutorEntry->first,
          prevExecutorEntry->second));
    }

    // Check if creation time is above threshold.
    if (resultPair.second == true) {
      time_t insertionTime = resultPair.first->second;
      if (timeNow - insertionTime >= this->threshold) {
        ResourceUsage_Executor* newExec = new ResourceUsage_Executor(executor);
        newUsage.mutable_executors()->AddAllocated(newExec);
      } else {
        continue;
      }
    } else {
      LOG(ERROR) << "IgnoreNewTasksFilter: "
                 << "Insert inside executor database failed.";
    }
  }

  this->executorTimestamps->clear();
  this->executorTimestamps = std::move(newExecutorTimestamps);

  if (0 != newUsage.executors_size()) {
    produce(newUsage);
  }

  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
