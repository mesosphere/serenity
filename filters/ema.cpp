#include <utility>

#include "filters/ema.hpp"

#include "serenity/metrics_helper.hpp"
#include "serenity/wid.hpp"

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
    if (!inExec.has_executor_info()) {
      SERENITY_LOG(ERROR) << "Executor <unknown>"
                 << " does not include executor_info";
      // Filter out these executors.
      continue;
    }
    if (!inExec.has_statistics()) {
      SERENITY_LOG(ERROR) << "Executor "
                 << inExec.executor_info().executor_id().value()
                 << " does not include statistics.";
      // Filter out these executors.
      continue;
    }
    newSamples->insert(inExec);

    // Check if EMA for given executor exists.
    auto emaSample = this->emaSamples->find(inExec.executor_info());
    if (emaSample == this->emaSamples->end()) {
      SERENITY_LOG(ERROR) << "First EMA iteration for: "
                          << WID(inExec.executor_info()).toString();
      // If not insert new one.
      ExponentialMovingAverage ema(EMA_REGULAR_SERIES, this->alpha);
      emaSamples->insert(std::pair<ExecutorInfo, ExponentialMovingAverage>(
          inExec.executor_info(), ema));

    } else {
      // Check if previousSample for given executor exists.
      auto previousSample = this->previousSamples->find(inExec);
      if (previousSample != this->previousSamples->end()) {
        // Get proper value.
        Try<double_t> value = this->valueGetFunction((*previousSample), inExec);
        if (value.isError()) {
          SERENITY_LOG(ERROR) << value.error();
          continue;
        }

        // Perform EMA filtering.
        double_t emaValue =
          (emaSample->second).calculateEMA(
              value.get(),
              inExec.statistics().perf().timestamp());

        // Store EMA value.
        ResourceUsage_Executor* outExec = new ResourceUsage_Executor(inExec);
        Try<Nothing> result = this->valueSetFunction(emaValue, outExec);
        if (result.isError()) {
          SERENITY_LOG(ERROR) << result.error();
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

  if (0 != product.executors_size()) {
    SERENITY_LOG(INFO) << "Continuing with "
                       << product.executors_size() << " executor(s).";
    // Continue pipeline.
    produce(product);
  }

  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
