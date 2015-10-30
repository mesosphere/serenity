#ifndef TESTS_SERENITY_HPP
#define TESTS_SERENITY_HPP

#include <gtest/gtest.h>

#include <mesos/mesos.pb.h>
#include <mesos/resources.hpp>

#include <process/future.hpp>

#include <list>
#include <string>

namespace mesos {
namespace serenity {
namespace tests {


#define DEFAULT_EXECUTOR_INFO                                           \
      ({ ExecutorInfo executor;                                         \
        executor.mutable_executor_id()->set_value("default");           \
        executor.mutable_command()->set_value("exit 1");                \
        executor; })

#define CREATE_EXECUTOR_INFO(executorId, command)                       \
      ({ ExecutorInfo executor;                                         \
        executor.mutable_executor_id()->set_value(executorId);          \
        executor.mutable_command()->set_value(command);                 \
        executor; })


// Factory for statistics stubs.
class ResourceHelper {
 public:
  // TODO(bplotka) parametrize that
  static ResourceStatistics createStatistics() {
    ResourceStatistics statistics;
    statistics.set_cpus_nr_periods(100);
    statistics.set_cpus_nr_throttled(2);
    statistics.set_cpus_user_time_secs(4);
    statistics.set_cpus_system_time_secs(1);
    statistics.set_cpus_throttled_time_secs(0.5);
    statistics.set_cpus_limit(1.0);
    statistics.set_mem_file_bytes(0);
    statistics.set_mem_anon_bytes(0);
    statistics.set_mem_mapped_file_bytes(0);
    statistics.set_mem_rss_bytes(1024);
    statistics.set_mem_limit_bytes(2048);
    statistics.set_timestamp(0);

    return statistics;
  }

  static void addExecutor(
      ResourceUsage& usage,
      ExecutorInfo executorInfo,
      Resources allocated,
      ResourceStatistics statistics) {
    ResourceUsage::Executor* executor = usage.add_executors();
    executor->mutable_executor_info()->CopyFrom(executorInfo);
    executor->mutable_allocated()->CopyFrom(allocated);
    executor->mutable_statistics()->CopyFrom(statistics);
  }
};


// Fake usage function (same method as in mesos::slave::Slave).
class MockSlaveUsage {
 public:
  MockSlaveUsage(int executors) : results(ResourceUsage()) {
    for (int i = 0; i < executors; i++) {
      ResourceHelper::addExecutor(
          results,
          CREATE_EXECUTOR_INFO(std::to_string(i + 1), "exit 1"),
          Resources(),
          ResourceHelper::createStatistics());
    }
  }

  process::Future<ResourceUsage> usage() {
    return results;
  }

 private:
  ResourceUsage results;
};

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

#endif  // TESTS_SERENITY_HPP
