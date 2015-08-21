#ifndef SERENITY_EXPONENTIAL_MOVING_AVERAGE_FILTER_HPP
#define SERENITY_EXPONENTIAL_MOVING_AVERAGE_FILTER_HPP

#include <glog/logging.h>

#include <cmath>
#include <memory>
#include <string>

#include "messages/serenity.hpp"

#include "serenity/data_utils.hpp"
#include "serenity/default_vars.hpp"
#include "serenity/executor_map.hpp"
#include "serenity/executor_set.hpp"
#include "serenity/serenity.hpp"

#include "stout/lambda.hpp"
#include "stout/nothing.hpp"

namespace mesos {
namespace serenity {

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
      double_t _alpha = ema::DEFAULT_ALPHA)
      : alpha(_alpha),
        seriesType(_seriesType),
        uninitialized(true) {}

  void setAlpha(double_t _alpha) {
    alpha = _alpha;
  }

  double_t getAlpha() const {
    return alpha;
  }

  /**
   * Calculate EMA and save needed values for next calculation.
   */
  double_t calculateEMA(double_t sample, double_t sampleTimestamp);

 private:
  //! Constant describing how the window weights decrease over time.
  //! It controls how long the moving average period is.
  //! The smaller alpha becomes, the longer your moving average is.
  //! It becomes smoother, but less reactive to new samples.
  double_t alpha;
  //! Previous exponential moving average value.
  double_t prevEma;
  //! Previous sample.
  double_t prevSample;
  //! Used for counting deltaTime.
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
      double_t sample, double_t sampleTimestamp) const;

  /**
   * Exponential Moving Average for regular time series.
   * Sample should be normalized always to the same unit.
   */
  double_t exponentialMovingAverageRegular(double_t sample) const;
};

/**
 * EMAFilter is able to calculate Exponential Moving Average on
 * ResourceUsage. Classes based on EMAFilter can define filter
 * for smoothing defined value.
 *
 * It is possible to calculate EMA on any value. For every value
 * separate Resource Usage getter and setter function have to be
 * implemented to fetch specified value and store it. It can be defined
 * in serenity/data_utils.hpp
 */
class EMAFilter :
    public Consumer<ResourceUsage>, public Producer<ResourceUsage> {
 public:
  EMAFilter(
      Consumer<ResourceUsage>* _consumer,
      const lambda::function<usage::GetterFunction>& _valueGetFunction,
      const lambda::function<usage::SetterFunction>& _valueSetFunction,
      double_t _alpha = ema::DEFAULT_ALPHA,
      const Tag& _tag = Tag(UNDEFINED, "emaFilter"))
    : tag(_tag), Producer<ResourceUsage>(_consumer),
      previousSamples(new ExecutorSet),
      emaSamples(new ExecutorMap<ExponentialMovingAverage>()),
      valueGetFunction(_valueGetFunction),
      valueSetFunction(_valueSetFunction),
      alpha(_alpha) {}

  ~EMAFilter() {}

  Try<Nothing> consume(const ResourceUsage& in);

 protected:
  const Tag tag;
  double_t alpha;
  const lambda::function<usage::GetterFunction> valueGetFunction;
  const lambda::function<usage::SetterFunction> valueSetFunction;
  std::unique_ptr<ExecutorSet> previousSamples;
  std::unique_ptr<ExecutorMap<ExponentialMovingAverage>> emaSamples;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EXPONENTIAL_MOVING_AVERAGE_FILTER_HPP
