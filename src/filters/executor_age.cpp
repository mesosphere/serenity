#include <atomic>
#include <string>

#include "glog/logging.h"

#include "mesos/mesos.hpp"

#include "serenity/executor_map.hpp"

#include "executor_age.hpp"

namespace mesos {
namespace serenity {

using std::map;
using std::pair;
using std::string;

ExecutorAgeFilter::ExecutorAgeFilter() : started(new ExecutorMap<double_t>()) {}


ExecutorAgeFilter::ExecutorAgeFilter(Consumer<ResourceUsage>* _consumer)
  : Producer<ResourceUsage>(_consumer), started(new ExecutorMap<double_t>()) {}


ExecutorAgeFilter::~ExecutorAgeFilter() {}


Try<Nothing> ExecutorAgeFilter::consume(const ResourceUsage& in)
{
  double_t now = time(NULL);
  
  for (ResourceUsage_Executor executor : in.executors()) {
    auto startedTime = this->started->find(executor.executor_info());
    if (startedTime == this->started->end()) {
        // If executor is missing, create start entry for executor.
      this->started->insert(pair<ExecutorInfo, double_t>(
          executor.executor_info(), now));
      this->age( executor.executor_info()); //For test!
    }
  }
  // TODO(nnielsen): Clean up finished frameworks and executors.

  this->produce(in);
  return Nothing();
}


Try<double_t> ExecutorAgeFilter::age(const ExecutorInfo& executorInfo)
{
  // Make core dump;
  LOG(INFO) << "Before core dump";
  auto startedTime = started->find(executorInfo);
  LOG(INFO) << "Log not visible";

  if (startedTime == started->end()) {
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
