#include <utility>

#include "filters/ema.hpp"

#include "serenity/metrics_helper.hpp"

namespace mesos {
namespace serenity {

double_t ExponentialMovingAverage::calculateEMA(
    double_t sample, double_t sampleTimestamp) {
  if (this->uninitialized) {
    this->prevEma = sample;
    this->uninitialized = false;
  }

  switch (seriesType) {
    case EMA_REGULAR_SERIES:
      this->prevEma = this->exponentialMovingAverageRegular(sample);
      break;
    case EMA_IRRERGULAR_SERIES:
      // TODO(bplotka): Test irregular series EMA.
      this->prevEma = this->exponentialMovingAverageIrregular(
          sample, sampleTimestamp);
      break;
  }

  this->prevSample = sample;
  this->prevSampleTimestamp = sampleTimestamp;

  return prevEma;
}


double_t ExponentialMovingAverage::exponentialMovingAverageIrregular(
    double_t sample, double_t sampleTimestamp) const {
  double_t deltaTime = sampleTimestamp - this->prevSampleTimestamp;
  double_t dynamicAlpha = deltaTime / this->alpha;
  double_t weight = exp(dynamicAlpha * -1);
  double_t dynamicWeight = (1 - weight) / dynamicAlpha;
  return (weight * this->prevEma) + (( dynamicWeight - weight )
          * this->prevSample) + ((1.0 - dynamicWeight) * sample);
}


double_t ExponentialMovingAverage::exponentialMovingAverageRegular(
    double_t sample) const {
  return (this->alpha * sample) + ((1 - this->alpha) * this->prevEma);
}


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
        Try<Nothing> result = emaTypeFunction(
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


Try<Nothing> EMATypes::filterIpc(
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


Try<Nothing> EMATypes::filterCpuUsage(
    ExponentialMovingAverage* ema,
    const ResourceUsage_Executor& previousExec,
    const ResourceUsage_Executor& currentExec,
    ResourceUsage_Executor* outExec) {
  Try<double_t> CpuUsage =
      CountCpuUsage(previousExec, currentExec);
  if (CpuUsage.isError()) return Error(CpuUsage.error());

  double_t emaCpuUsage =
      ema->calculateEMA(CpuUsage.get(), currentExec.statistics().timestamp());

  outExec->mutable_statistics()->set_net_tcp_time_wait_connections(
      emaCpuUsage);

  return Nothing();
}


}  // namespace serenity
}  // namespace mesos
