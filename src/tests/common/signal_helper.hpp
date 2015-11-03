#ifndef SERENITY_TEST_SIGNAL_HELPER_HPP
#define SERENITY_TEST_SIGNAL_HELPER_HPP

namespace mesos {
namespace serenity {
namespace tests {

/**
 * This function fills instructions and cycles perf events to match given
 * IPC value.
 */
inline ResourceUsage_Executor generateIPC(
    const ResourceUsage_Executor _executorUsage,
    double_t _IpcValue,
    double_t _timestamp) {
  const int ACCURACY_MODIFIER = 10;
  // This function generates IPC with 1/ACCURACY_MODIFIER accuracy.
  _IpcValue *= ACCURACY_MODIFIER;
  ResourceUsage_Executor executorUsage;
  executorUsage.CopyFrom(_executorUsage);

  executorUsage.mutable_statistics()
    ->mutable_perf()->set_instructions(_IpcValue);

  executorUsage.mutable_statistics()
    ->mutable_perf()->set_cycles(ACCURACY_MODIFIER);

  executorUsage.mutable_statistics()
    ->mutable_perf()->set_timestamp(_timestamp);

  return executorUsage;
}


/**
* This function fills instructions perf events to match given
* IPC value.
*/
inline ResourceUsage_Executor generateIPS(
    const ResourceUsage_Executor _executorUsage,
    double_t _IpsValue,
    double_t _timestamp) {
  const int ACCURACY_MODIFIER = 10;
  // This function generates IPS with 1/ACCURACY_MODIFIER accuracy.
  _IpsValue *= ACCURACY_MODIFIER;
  ResourceUsage_Executor executorUsage;
  executorUsage.CopyFrom(_executorUsage);

  executorUsage.mutable_statistics()
    ->mutable_perf()->set_instructions(_IpsValue);

  executorUsage.mutable_statistics()
    ->mutable_perf()->set_duration(ACCURACY_MODIFIER);

  executorUsage.mutable_statistics()
    ->mutable_perf()->set_timestamp(_timestamp);

  return executorUsage;
}


/**
* This function fills statistics cpu system time to match given
* CPU value.
*/
inline ResourceUsage_Executor generateCpuUsage(
    const ResourceUsage_Executor _executorUsage,
    uint64_t _cumulativeCpuTimeValue,
    double_t _timestamp) {
  ResourceUsage_Executor executorUsage;
  executorUsage.CopyFrom(_executorUsage);

  executorUsage.mutable_statistics()
    ->set_cpus_system_time_secs(_cumulativeCpuTimeValue);

  executorUsage.mutable_statistics()
    ->set_cpus_user_time_secs(0);

  executorUsage.mutable_statistics()
    ->set_timestamp(_timestamp);

  return executorUsage;
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

#endif //SERENITY_TEST_SIGNAL_HELPER_HPP
