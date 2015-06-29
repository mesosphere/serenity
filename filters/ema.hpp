#ifndef SERENITY_EXPONENTIAL_MOVING_AVERAGE_HPP
#define SERENITY_EXPONENTIAL_MOVING_AVERAGE_HPP

#include <math.h>

#include "serenity/serenity.hpp"

#include "stout/option.hpp"

namespace mesos {
namespace serenity {

// The smaller alpha becomes, the longer your moving average is.
// It becomer smoother, but less reactive to new samples.
#define DEFAULT_EMA_FILTER_ALPHA 0.2

class ExponentialMovingAverage {
 public:
  ExponentialMovingAverage(
        Option<double_t> _alpha)
      : uninitialized(true) {
    if (_alpha.isSome()) {
      setAlpha(_alpha.get());
    } else {
      setAlpha(DEFAULT_EMA_FILTER_ALPHA);
    }
  }

  void setAlpha(double_t _alpha) {
    alpha = _alpha;
  }

  double_t getAlpha() {
    return alpha;
  }

  /*
  * Calculate EMA and save needed values for next calculation.
  */
  double_t calculateEMA(double_t sample, double_t sampleTimestamp) {
    if (uninitialized) {
      prevEma = sample;
      uninitialized = false;
    }
    prevEma = exponentialMovingAverageRegular(sample);
    prevSample = sample;
    prevSampleTimestamp = sampleTimestamp;

    return prevEma;
  }

 private:
  // Constant describing how the window weights decrease over time.
  double_t alpha;
  // Previous exponential moving average value.
  double_t prevEma;
  // Previous sample.
  double_t prevSample;
  // Used for counting deltaTime.
  double_t prevSampleTimestamp;
  bool uninitialized;

  /*
   * Exponential Moving Average for irregular time series.
   * TODO(bplotka): Test it - strong dependence on time normalization.
   * Inspired by: oroboro.com/irregular-ema/
   * Timestamp is absolute.
   * Sample should be normalized always to the same unit.
   */
  double_t exponentialMovingAverageIrregular(double_t sample, double_t
  sampleTimestamp) {
    double_t deltaTime = sampleTimestamp - prevSampleTimestamp;
    double_t dynamicAlpha = deltaTime / alpha;
    double_t weight = exp(dynamicAlpha * -1);
    double_t dynamicWeight = (1 - weight) / dynamicAlpha;
    return (weight * prevEma) + (( dynamicWeight - weight ) * prevSample) +
           ((1.0 - dynamicWeight) * sample);
  }

  /*
   * Exponential Moving Average for regular time series.
   * Sample should be normalized always to the same unit.
   */
  double_t exponentialMovingAverageRegular(double_t sample) {
    return (alpha*sample) + ((1 - alpha)*prevEma);
  }
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EXPONENTIAL_MOVING_AVERAGE_HPP
