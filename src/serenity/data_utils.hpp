#ifndef SERENITY_DATA_UTILS_HPP
#define SERENITY_DATA_UTILS_HPP

#include "serenity/metrics_helper.hpp"
#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {
namespace usage {

//! Resource Usage getters.
using GetterFunction = Try<double_t>(
    const ResourceUsage_Executor& currentExec);


inline Try<double_t> getIpc(
    const ResourceUsage_Executor& currentExec) {
  Try<double_t> ipc = CountIpc(currentExec);
  if (ipc.isError()) return Error(ipc.error());

  return ipc;
}


/**
 * Currently we are saving EMA IPC in net_tcp_active_connections field
 * in ResourceUsage.
 */
inline Try<double_t> getEmaIpc(
    const ResourceUsage_Executor& currentExec) {
  if (!currentExec.statistics().has_net_tcp_active_connections())
    return Error("Ema IPC is not filled");

  return currentExec.statistics().net_tcp_active_connections();
}


inline Try<double_t> getIps(
    const ResourceUsage_Executor& currentExec) {
  Try<double_t> ips = CountIps(currentExec);
  if (ips.isError()) return Error(ips.error());

  return ips;
}

// TODO(bplotka): Move it to other double field.
/**
 * Currently we are saving EMA IPS in net_tcp_active_connections field
 * in ResourceUsage.
 */
inline Try<double_t> getEmaIps(
    const ResourceUsage_Executor& currentExec) {
  if (!currentExec.statistics().has_net_tcp_active_connections())
    return Error("Ema IPS is not filled");

  return currentExec.statistics().net_tcp_active_connections();
}


inline Try<double_t> getCpuUsage(
    const ResourceUsage_Executor& currentExec) {
  Try<double_t> cpuUsage =
      CountSampledCpuUsage(currentExec);
  if (cpuUsage.isError()) return Error(cpuUsage.error());

  return cpuUsage;
}


/**
 * Currently we are saving EMA CpuUsage in net_tcp_time_wait_connections field
 * in ResourceUsage.
 */
inline Try<double_t> getEmaCpuUsage(
    const ResourceUsage_Executor& currentExec) {
  if (!currentExec.statistics().has_net_tcp_time_wait_connections())
    return Error("Ema CpuUsage is not filled");

  return currentExec.statistics().net_tcp_time_wait_connections();
}


//! Resource Usage setters.
using SetterFunction = Try<Nothing>(
    const double_t value,
    ResourceUsage_Executor* outExec);


/**
 * Currently we are saving EMA IPC in net_tcp_active_connections field
 * in ResourceUsage.
 */
inline Try<Nothing> setEmaIpc(
    const double_t value,
    ResourceUsage_Executor* outExec) {

  outExec->mutable_statistics()->set_net_tcp_active_connections(value);

  return Nothing();
}

// TODO(bplotka): Move it to other double field.
/**
 * Currently we are saving EMA IPS in net_tcp_active_connections field
 * in ResourceUsage.
 */
inline Try<Nothing> setEmaIps(
    const double_t value,
    ResourceUsage_Executor* outExec) {

  outExec->mutable_statistics()->set_net_tcp_active_connections(value);

  return Nothing();
}


/**
 * Currently we are saving EMA CpuUsage in net_tcp_time_wait_connections field
 * in ResourceUsage.
 */
inline Try<Nothing> setEmaCpuUsage(
    const double_t value,
    ResourceUsage_Executor* outExec) {

  outExec->mutable_statistics()->set_net_tcp_time_wait_connections(value);

  return Nothing();
}

}  // namespace usage
}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_DATA_UTILS_HPP
