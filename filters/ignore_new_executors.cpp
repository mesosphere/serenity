#include "filters/ignore_new_executors.hpp"

#include "glog/logging.h"

namespace mesos {
namespace serenity {

Try<Nothing> IgnoreNewExecutorsFilter::consume(const ResourceUsage &usage) {
  std::unique_ptr<ExecutorMap<time_t>> newExecutorTimestamps =
    std::unique_ptr<ExecutorMap<time_t>>(new ExecutorMap<time_t>());
  ResourceUsage product;

  time_t timeNow = this->GetTime(nullptr);
  // insert method result: tuple<Iterator, bool>
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

    // Check if insertion was successful
    if (resultPair.second == true) {
      time_t insertionTime = resultPair.first->second;
      // Check if insertion time is above threshold
      if (timeNow - insertionTime >= this->threshold) {
        ResourceUsage_Executor* newExec = product.mutable_executors()->Add();
        newExec->CopyFrom(executor);
      }
    } else {
      LOG(ERROR) << "IgnoreNewTasksFilter: "
                 << "Insert inside executor database failed.";
    }
  }

  this->executorTimestamps->clear();
  this->executorTimestamps = std::move(newExecutorTimestamps);

  if (0 != product.executors_size()) {
    produce(product);
  }

  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
