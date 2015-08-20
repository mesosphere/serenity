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

ExecutorAgeFilter::ExecutorAgeFilter() {}


ExecutorAgeFilter::ExecutorAgeFilter(Consumer<ResourceUsage>* _consumer)
  : Producer<ResourceUsage>(_consumer) {}


ExecutorAgeFilter::~ExecutorAgeFilter() {}


Try<Nothing> ExecutorAgeFilter::consume(const ResourceUsage& in)
{
  time_t now = time(NULL);
  
  for (ResourceUsage_Executor executor : in.executors()) {
    const FrameworkID& frameworkId = executor.executor_info().framework_id();
    const ExecutorID& executorId = executor.executor_info().executor_id();

    FrameworkMap::iterator frameworkIterator =
      started.find(frameworkId.value());

    if (frameworkIterator == started.end()) {
      // If executor map for framework is absent, create and initialize the
      // entry.
      map<const string, time_t> executors;
      executors.insert(pair<const string, time_t>(executorId.value(), now));

      started.insert(pair<const string, map<const string, time_t>>(
        frameworkId.value(), executors));
    } else {
      ExecutorMap& executors = frameworkIterator->second;
        
      ExecutorMap::iterator executorIterator =
        executors.find(executorId.value());

      if (executorIterator == executors.end()) {
        // If executor is missing, create start entry for executor.
        executors.insert(pair<const string, time_t>(executorId.value(), now));
      }
    }
  }

  // TODO(nnielsen): Clean up finished frameworks and executors.

  this->produce(in);
  return Nothing();
}


Try<double> ExecutorAgeFilter::age(
  const FrameworkID& frameworkId,
  const ExecutorID& executorId)
{
  FrameworkMap::iterator frameworkIterator =
    started.find(frameworkId.value());

  if (frameworkIterator == started.end()) {
    return 0;
//    return Error(
//      "Could not find started time for executor '" + frameworkId.value() +
//      "' of framework '" + executorId.value() + "': framework not present");
  }

  ExecutorMap& executors = frameworkIterator->second;
  ExecutorMap::iterator executorIterator = executors.find(executorId.value());

  if (executorIterator == executors.end()) {
    return 0;
//    return Error(
//      "Could not find started time for executor '" + frameworkId.value() +
//      "' of framework '" + executorId.value() + "': executor not present");
  }

  return difftime(time(NULL), executorIterator->second);
}


}  // namespace serenity
}  // namespace mesos
