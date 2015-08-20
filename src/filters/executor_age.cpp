#include <atomic>
#include <string>

#include "glog/logging.h"

#include "mesos/mesos.hpp"

#include "executor_age.hpp"

namespace mesos {
namespace serenity {

using std::map;
using std::pair;
using std::string;

ExecutorAgeFilter::ExecutorAgeFilter() : started(new ExecutorMap<time_t>()) {}


ExecutorAgeFilter::ExecutorAgeFilter(Consumer<ResourceUsage>* _consumer)
  : Producer<ResourceUsage>(_consumer), started(new ExecutorMap<time_t>()) {}


ExecutorAgeFilter::~ExecutorAgeFilter() {}


Try<Nothing> ExecutorAgeFilter::consume(const ResourceUsage& in)
{
  time_t now = time(NULL);
  
  for (ResourceUsage_Executor executor : in.executors()) {
    auto startedTime = this->started->find(executor.executor_info());
    if (startedTime == this->started->end()) {
        // If executor is missing, create start entry for executor.
      this->started->insert(pair<ExecutorInfo, time_t>(
          executor.executor_info(), now));
    }
  }

  // TODO(nnielsen): Clean up finished frameworks and executors.

  this->produce(in);
  return Nothing();
}


Try<double> ExecutorAgeFilter::age(
  const ExecutorInfo& executorInfo)
{
  auto startedTime = this->started->find(executorInfo);
  if (startedTime == this->started->end()) {
    return Error(
        "Could not find started time for executor '" +
        executorInfo.framework_id().value() + "' of framework '" +
        executorInfo.executor_id().value() + "': framework not present");
  } else {
    return difftime(time(NULL), startedTime->second);
  }

}

}  // namespace serenity
}  // namespace mesos
