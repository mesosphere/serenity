#include <algorithm>
#include <list>
#include <utility>

#include "bus/event_bus.hpp"

#include "filters/executor_age.hpp"

#include "observers/strategies/cpu_contention.hpp"

#include "serenity/resource_helper.hpp"

namespace mesos {
namespace serenity {

using std::list;
using std::pair;

Try<QoSCorrections> CpuContentionStrategy::decide(
    ExecutorAgeFilter* ageFilter,
    const Contentions& currentContentions,
    const ResourceUsage& currentUsage) {
  // Product.
  QoSCorrections corrections;

  // List of revocable executors.
  list<ResourceUsage_Executor> revocableExecutors =
    ResourceUsageHelper::getRevocableExecutors(currentUsage);

  // Aggressors to be killed. (empty for now).
  std::list<slave::QoSCorrection_Kill> executorsToRevoke;

  // Amount of CPUs to satisfy the contention.
  double_t cpuToRecover = 0.0;
  for (const Contention contention : currentContentions) {
    if (contention.type() != Contention_Type_CPU) {
      SERENITY_LOG(ERROR) << "Cannot decide about contentions type other than"
                          << " CPU. Omitting instance";
    }

    // In case of Contention_Type_CPU severity value
    // means amount of CPUs to recover.
    if (contention.has_severity()) {
      cpuToRecover = std::max(contention.severity(), cpuToRecover);
    } else {
      cpuToRecover = std::max(this->defaultSeverity, cpuToRecover);
    }
  }

  SERENITY_LOG(INFO) << "Cpus to recover from revocable tasks: "
                     << cpuToRecover;

  // Check for cpus limits violations and set age.
  list<pair<double_t, ResourceUsage_Executor>> executors;
  for (const ResourceUsage_Executor& executor : revocableExecutors) {
    if (cpuToRecover <= 0) break;

    Try<double_t> value = this->getCpuUsage(executor);
    if (value.isError()) {
      SERENITY_LOG(ERROR) << value.error();
      continue;
    }

    if (value.get() > Resources(executor.allocated()).cpus().get()) {
      // Executor exceeds its limits, mark it for revocation.
      const ExecutorInfo& executorInfo = executor.executor_info();
      SERENITY_LOG(INFO) << "Marked executor '" << executorInfo.executor_id()
      << "' of framework '" << executorInfo.framework_id()
      << "' because of limit violation";

      executorsToRevoke.push_back(createKill(executorInfo));

      // Recover cpus.
      cpuToRecover -= value.get();
      continue;
    }

    Try<double_t> age = ageFilter->age(executor.executor_info());
    if (age.isError()) {
      LOG(WARNING) << age.error();
      continue;
    }

    executors.push_back(
      pair<double_t, ResourceUsage_Executor>(age.get(), executor));
  }

  if (cpuToRecover > 0) {
    // TODO(nielsen): Actual time delta should be factored in i.e. not only work
    // as an ordering, but as a priority (taken time gaps).
    executors.sort([](
      const pair<double_t, ResourceUsage_Executor>& left,
      const pair<double_t, ResourceUsage_Executor>& right){
      return left.first < right.first;
    });

    for (auto executor : executors) {
      if (cpuToRecover <= 0) break;

      const ExecutorInfo& executorInfo =
        (executor.second).executor_info();

      SERENITY_LOG(INFO) << "Marked executor '" << executorInfo.executor_id()
      << "' of framework '" << executorInfo.framework_id()
      << "' age " << executor.first << "s for removal";

      executorsToRevoke.push_back(createKill(executorInfo));

      // TODO(bplotka): Use Cpu Usage, EMA Cpu Usage or allocated cpus?
      // IMO EMA CPU usage is the most accurate here...
      Try<double_t> value = this->getCpuUsage(executor.second);
      double_t recoveredCpus = 0.0;
      if (value.isError()) {
        SERENITY_LOG(ERROR) << value.error();
        recoveredCpus = Resources(executor.second.allocated()).cpus().get();
      } else {
        recoveredCpus = value.get();
      }

      cpuToRecover -= recoveredCpus;
    }
  }

  // Create QoSCorrection from aggressors list.
  for (auto aggressorToKill : executorsToRevoke) {
    corrections.push_back(createKillQoSCorrection(aggressorToKill));
  }

  return corrections;
}

}  // namespace serenity
}  // namespace mesos
