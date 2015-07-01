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

/**
 * EMA calculation can be more sophisticated for irregular
 * time series, since it can influence how 'old' are our previous samples.
 * We can choose between two algorithms.
 */
enum EMASeriesType {
  EMA_REGULAR_SERIES,
  EMA_IRRERGULAR_SERIES
};


class ExponentialMovingAverage {
 public:
  ExponentialMovingAverage(
      EMASeriesType _seriesType = EMA_REGULAR_SERIES,
      double_t _alpha = DEFAULT_EMA_FILTER_ALPHA)
      : alpha(_alpha),
        seriesType(_seriesType),
      uninitialized(true) {}

  void setAlpha(double_t _alpha) {
    alpha = _alpha;
  }

  double_t getAlpha() {
    return alpha;
  }

  /**
  * Calculate EMA and save needed values for next calculation.
  */
  double_t calculateEMA(double_t sample, double_t sampleTimestamp) {
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

 private:
  // Constant describing how the window weights decrease over time.
  double_t alpha;
  // Previous exponential moving average value.
  double_t prevEma;
  // Previous sample.
  double_t prevSample;
  // Used for counting deltaTime.
  double_t prevSampleTimestamp;
  EMASeriesType seriesType;
  bool uninitialized;

  /**
   * Exponential Moving Average for irregular time series.
   * TODO(bplotka): Test it - strong dependence on time normalization.
   * Inspired by: oroboro.com/irregular-ema/
   * Timestamp is absolute.
   * Sample should be normalized always to the same unit.
   */
  double_t exponentialMovingAverageIrregular(
      double_t sample, double_t sampleTimestamp) const {
    double_t deltaTime = sampleTimestamp - this->prevSampleTimestamp;
    double_t dynamicAlpha = deltaTime / this->alpha;
    double_t weight = exp(dynamicAlpha * -1);
    double_t dynamicWeight = (1 - weight) / dynamicAlpha;
    return (weight * this->prevEma) + (( dynamicWeight - weight )
           * this->prevSample) + ((1.0 - dynamicWeight) * sample);
  }

  /**
   * Exponential Moving Average for regular time series.
   * Sample should be normalized always to the same unit.
   */
  double_t exponentialMovingAverageRegular(double_t sample) const {
    return (this->alpha * sample) + ((1 - this->alpha) * this->prevEma);
  }
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EXPONENTIAL_MOVING_AVERAGE_HPP
