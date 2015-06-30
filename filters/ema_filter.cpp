#include "ema_filter.hpp"

#include <utility>

namespace mesos {
namespace serenity {


Try<Nothing> EMAFilter::consume(const ResourceUsage& in) {
  std::unique_ptr<ExecutorSet> newSamples(new ExecutorSet());
  ResourceUsage product;

  for (ResourceUsage_Executor inExec : in.executors()) {
    if (!inExec.has_statistics() || !inExec.has_executor_info()) {
      LOG(ERROR) << "Executor "
                    << inExec.executor_info().executor_id().value()
                    << " has not proper statistics or executor_info";
    }
    newSamples->insert(inExec);

    // Check if EMA for given executor exists.
    auto emaSample = this->emaSamples->find(inExec.executor_info());
    if (emaSample == this->emaSamples->end()) {
      // If not insert new one.
      ExponentialMovingAverage ema;
      emaSamples->insert(std::pair<ExecutorInfo, ExponentialMovingAverage>(
          inExec.executor_info(), ema));

    } else {
      // Check if previousSample for given executor exists.
      auto previousSample = this->previousSamples->find(inExec);
      if (previousSample != this->previousSamples->end()) {
        ResourceUsage_Executor* outExec(
            new ResourceUsage_Executor());

        outExec->CopyFrom(inExec);

        // Perform EMA filtering.
        Try<Nothing> result = filter(
            &((*emaSample).second), (*previousSample), inExec, outExec);
        if (result.isError()) {
          LOG(ERROR) << result.error();
          delete outExec;
          continue;
        }

        // Add an executor only when there was no error.
        product.mutable_executors()->AddAllocated(outExec);
      }
    }
  }

  this->previousSamples->clear();
  this->previousSamples = std::move(newSamples);

  // Send only when some executors are present.
  if (product.executors().size() != 0)
    produce(product);

  return Nothing();
}


IpcEMAFilter::~IpcEMAFilter() {}


Try<Nothing> IpcEMAFilter::filter(
      ExponentialMovingAverage* ema,
      const ResourceUsage_Executor& previousExec,
      const ResourceUsage_Executor& currentExec,
      ResourceUsage_Executor* outExec) {
  Try<double_t> Ipc = CountIpc(previousExec, currentExec);
  if (Ipc.isError()) return Error(Ipc.error());

  double_t emaIpc =
    ema->calculateEMA(Ipc.get(), currentExec.statistics().perf().timestamp());

  outExec->mutable_statistics()->set_net_tcp_active_connections(emaIpc);

  return Nothing();
}


CpuUsageEMAFilter::~CpuUsageEMAFilter() {}


Try<Nothing> CpuUsageEMAFilter::filter(
    ExponentialMovingAverage* ema,
    const ResourceUsage_Executor& previousExec,
    const ResourceUsage_Executor& currentExec,
    ResourceUsage_Executor* outExec) {
  Try<double_t> CpuUsage = CountCpuUsage(previousExec, currentExec);
  if (CpuUsage.isError()) return Error(CpuUsage.error());

  double_t emaCpuUsage =
      ema->calculateEMA(CpuUsage.get(), currentExec.statistics().timestamp());

  outExec->mutable_statistics()->set_net_tcp_time_wait_connections(
      emaCpuUsage);

  return Nothing();
}


}  // namespace serenity
}  // namespace mesos
