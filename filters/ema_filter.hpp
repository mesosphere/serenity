#ifndef SERENITY_EXPONENTIAL_MOVING_AVERAGE_FILTER_HPP
#define SERENITY_EXPONENTIAL_MOVING_AVERAGE_FILTER_HPP

#include <math.h>

#include <bits/unique_ptr.h>

#include <glog/logging.h>

#include "filters/ema.hpp"

#include "messages/serenity.hpp"

#include "serenity/executor_map.hpp"
#include "serenity/executor_set.hpp"
#include "serenity/metrics_helper.hpp"
#include "serenity/serenity.hpp"

#include "stout/lambda.hpp"
#include "stout/nothing.hpp"

namespace mesos {
namespace serenity {

typedef Try<Nothing> (EMATypeFilterFunction)
    (ExponentialMovingAverage*,
     const ResourceUsage_Executor&,
     const ResourceUsage_Executor&,
     ResourceUsage_Executor*);


/**
 * It is possible to calculate EMA on any value. For every value
 * seperate filter function have to be implemented to fetch
 * specified valye and store it.
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
