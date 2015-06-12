/**
 * This file is © 2015 Mesosphere, Inc. ("Mesosphere"). Mesosphere
 * licenses this file to you solely pursuant to the agreement between
 * Mesosphere and you (if any).  If there is no such agreement between
 * Mesosphere, the following terms apply (and you may not use this
 * file except in compliance with such terms):
 *
 * 1) Subject to your compliance with the following terms, Mesosphere
 * hereby grants you a nonexclusive, limited, personal,
 * non-sublicensable, non-transferable, royalty-free license to use
 * this file solely for your internal business purposes.
 *
 * 2) You may not (and agree not to, and not to authorize or enable
 * others to), directly or indirectly:
 *   (a) copy, distribute, rent, lease, timeshare, operate a service
 *   bureau, or otherwise use for the benefit of a third party, this
 *   file; or
 *
 *   (b) remove any proprietary notices from this file.  Except as
 *   expressly set forth herein, as between you and Mesosphere,
 *   Mesosphere retains all right, title and interest in and to this
 *   file.
 *
 * 3) Unless required by applicable law or otherwise agreed to in
 * writing, Mesosphere provides this file on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied,
 * including, without limitation, any warranties or conditions of
 * TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4) In no event and under no legal theory, whether in tort
 * (including negligence), contract, or otherwise, unless required by
 * applicable law (such as deliberate and grossly negligent acts) or
 * agreed to in writing, shall Mesosphere be liable to you for
 * damages, including any direct, indirect, special, incidental, or
 * consequential damages of any character arising as a result of these
 * terms or out of the use or inability to use this file (including
 * but not limited to damages for loss of goodwill, work stoppage,
 * computer failure or malfunction, or any and all other commercial
 * damages or losses), even if Mesosphere has been advised of the
 * possibility of such damages.
 */

#ifndef TESTS_SERENITY_HPP
#define TESTS_SERENITY_HPP

#include <list>
#include <string>

#include <gtest/gtest.h>

#include <mesos/mesos.pb.h>

namespace mesos {
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
class ResourceHelper
{
public:
  // TODO(bplotka) parametrize that
  static ResourceStatistics createStatistics()
  {
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
      ResourceStatistics statistics)
  {
    ResourceUsage::Executor* executor = usage.add_executors();
    executor->mutable_executor_info()->CopyFrom(executorInfo);
    executor->mutable_allocated()->CopyFrom(allocated);
    executor->mutable_statistics()->CopyFrom(statistics);
  }
};


// Fake usages function (same method as in mesos::slave::Slave).
class MockSlaveUsage
{
public:
  MockSlaveUsage(int executors) : results(ResourceUsage())
  {
    for (int i = 0; i < executors; i++) {
      ResourceHelper::addExecutor(
          results,
          CREATE_EXECUTOR_INFO(std::to_string(i + 1), "exit 1"),
          Resources(),
          ResourceHelper::createStatistics());
    }
  }

  process::Future<ResourceUsage> usages()
  {
    return results;
  }

private:
  ResourceUsage results;
};

} // tests {
} // mesos {

#endif //TESTS_SERENITY_HPP
