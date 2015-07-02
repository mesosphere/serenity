#ifndef SERENITY_EXPONENTIAL_MOVING_AVERAGE_FILTER_HPP
#define SERENITY_EXPONENTIAL_MOVING_AVERAGE_FILTER_HPP

#include <glog/logging.h>

#include <cmath>
#include <memory>

#include "messages/serenity.hpp"

#include "serenity/executor_map.hpp"
#include "serenity/executor_set.hpp"
#include "serenity/serenity.hpp"

#include "stout/lambda.hpp"
#include "stout/nothing.hpp"

namespace mesos {
namespace serenity {

/**
 * Alpha controls how long is the moving average period.
 * The smaller alpha becomes, the longer your moving average is.
 * It becomes smoother, but less reactive to new samples.
 */
constexpr double_t DEFAULT_EMA_FILTER_ALPHA = 0.2;


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

  double_t getAlpha() const {
    return alpha;
  }

  /**
   * Calculate EMA and save needed values for next calculation.
   */
  double_t calculateEMA(double_t sample, double_t sampleTimestamp);

 private:
  //! Constant describing how the window weights decrease over time.
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


using EMATypeFilterFunction = Try<Nothing>
    (ExponentialMovingAverage*,
     const ResourceUsage_Executor&,
     const ResourceUsage_Executor&,
     ResourceUsage_Executor*) noexcept;


/**
 * It is possible to calculate EMA on any value. For every value
 * seperate filter function have to be implemented to fetch
 * specified value and store it.
 *
 * - filterIpc calculates Exponential Moving Average of IPC.
 * It gets IPC from perf statistics in ResourceStatistics.
 * It stores the calculated value in statistics.net_tcp_active_connections
 * field.
 * NOTE: It requires perf enabled on slave node.
 *
 * - filterCpuUsage calculates Exponential Moving Average of cpu usage.
 * It gets CpuUsage from statistics in ResourceUsage_Executor.
 * It stores the calculated value in statistics.net_tcp_time_wait_connections
 * field.
 */
class EMATypes {
 public:
  static EMATypeFilterFunction filterIpc;

  static EMATypeFilterFunction filterCpuUsage;
};


/**
 * EMAFilter is able to calculate Exponential Moving Average on
 * ResourceUsage. Classes based on EMAFilter can define filter
 * for smoothing defined value.
 */
class EMAFilter :
    public Consumer<ResourceUsage>, public Producer<ResourceUsage> {
 public:
  EMAFilter(
      Consumer<ResourceUsage>* _consumer,
      const lambda::function<EMATypeFilterFunction>& _emaTypeFunction,
      double_t _alpha = DEFAULT_EMA_FILTER_ALPHA)
    : Producer<ResourceUsage>(_consumer),
      previousSamples(new ExecutorSet),
      emaSamples(new MapHelper<ExponentialMovingAverage>::ExecutorMap),
      emaTypeFunction(_emaTypeFunction),
      alpha(_alpha) {}

  ~EMAFilter() {}

  Try<Nothing> consume(const ResourceUsage& in);

 protected:
  double_t alpha;
  const lambda::function<EMATypeFilterFunction>& emaTypeFunction;
  std::unique_ptr<ExecutorSet> previousSamples;
  std::unique_ptr<MapHelper<ExponentialMovingAverage>::ExecutorMap> emaSamples;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EXPONENTIAL_MOVING_AVERAGE_FILTER_HPP
