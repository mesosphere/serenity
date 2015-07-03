#include <ctime>

#include "glog/logging.h"

#include "filters/ignore_new_tasks.hpp"

#include "stout/try.hpp"
#include "stout/nothing.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> IgnoreNewTasksFilter::consume(const ResourceUsage &usage) {
  std::unique_ptr<ExecutorMap<time_t>> newExecutorTimestamps =
    std::unique_ptr<ExecutorMap<time_t>>(new ExecutorMap<time_t>());
  ResourceUsage newUsage;

  time_t now = time(0);
  auto resultPair = std::make_pair(executorMap->begin() , true);
  for (const auto& executor : usage.executors()) {
    ExecutorInfo executorInfo = executor.executor_info();

    const auto& prevExecutorEntry = this->executorMap->find(executorInfo);
    if (prevExecutorEntry != this->executorMap->end()) {
      resultPair = newExecutorTimestamps->insert(std::make_pair(
          prevExecutorEntry->first,
          prevExecutorEntry->second
      ));
    } else {
      resultPair = newExecutorTimestamps->insert(
          std::make_pair(executor.executor_info(), now));
    }

    if(resultPair.second == true) {
//      if (resultPair.)
    } else {
      LOG(ERROR) << "IgnoreNewTasksFilter: "
                        "Insert inside executor database failed." << std::endl;
    }
  }

  return Nothing();
}

} // namespace serenity
} // namespace mesos
