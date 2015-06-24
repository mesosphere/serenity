#ifndef SERENITY_EXPONENTIAL_MOVING_AVERAGE_HPP
#define SERENITY_EXPONENTIAL_MOVING_AVERAGE_HPP

#include <math.h>

#include "messages/serenity.hpp"

#include "serenity/serenity.hpp"

#include "stout/option.hpp"

namespace mesos {
namespace serenity {

#define DEFAULT_EMA_FILTER_ALPHA 0.2

/**
 * EMAFilter returns Exponential Moving Average (double) for
 * given input.
 */
template <typename In>
class EMAFilter : public Consumer<In>, public Producer<double> {
 public:
  explicit EMAFilter(Option<double> _alpha) {
    if (_alpha.isSome()) {
      setAlpha(_alpha.get());
    } else {
      setAlpha(DEFAULT_EMA_FILTER_ALPHA);
    }
  }

  EMAFilter(Consumer<double>* _consumer, Option<double> _alpha) :
      Producer(_consumer) {
    if (_alpha.isSome()) {
      setAlpha(_alpha.get());
    } else {
      setAlpha(DEFAULT_EMA_FILTER_ALPHA);
    }
  }

  ~EMAFilter() {}

  virtual Try<Nothing> consume(const In& in) = 0;

  void setAlpha(double _alpha) {
    alpha = _alpha;
  }

  double getAlpha() {
    return alpha;
  }

 protected:
  // Constant describing how the window weights decrease over time.
  double alpha;
  // Previous exponential moving average value.
  double prevEma;
  // Previous sample.
  double prevSample;
  // Used for counting deltaTime.
  double prevSampleTimestamp;

  /*
   * Exponential Moving Average for irregular time series.
   * Inspired by: oroboro.com/irregular-ema/
   */
  double exponentialMovingAverage(double sample, double sampleTimestamp) {
    double deltaTime = sampleTimestamp - prevSampleTimestamp;
    double dynamic_alpha = deltaTime / alpha;
    double weight = exp(dynamic_alpha * -1);
    double dynamic_weight = (1 - weight) / dynamic_alpha;

    return (weight * prevEma) + (( dynamic_weight - weight ) * prevSample) +
      ((1.0 - dynamic_weight) * sample);
  }

  /*
   * Calculate EMA and save needed values for next calculation.
   */
  double calculateEMA(double sample, double sampleTimestamp) {
    prevEma = exponentialMovingAverage(sample, sampleTimestamp);
    prevSample = sample;
    prevSampleTimestamp = sampleTimestamp;

    return prevEma;
  }
};


/*
 * IpcEMAFilter returns Exponential Moving Average (double) for
 * IPC value.
 * It gets IPC from perf statistics in ResourceUsage_Executor.
 */
class IpcEMAFilter : EMAFilter<ResourceUsage_Executor> {
 public:
  IpcEMAFilter(Consumer<double>* _consumer, Option<double> _alpha) :
      EMAFilter(_consumer, _alpha) {}

  ~IpcEMAFilter();

  Try<Nothing> consume(const ResourceUsage_Executor& in);
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EXPONENTIAL_MOVING_AVERAGE_HPP
