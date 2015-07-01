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

#include "stout/result.hpp"

namespace mesos {
namespace serenity {

/**
 * EMAFilter is able to calculate Exponential Moving Average on
 * ResourceUsage. Classes based on EMAFilter can define filter
 * for smoothing defined value.
 */
class EMAFilter :
    public Consumer<ResourceUsage>, public Producer<ResourceUsage> {
 public:
  EMAFilter(Consumer<ResourceUsage>* _consumer, double_t _alpha) :
      Producer<ResourceUsage>(_consumer),
      previousSamples(new ExecutorSet),
      emaSamples(new MapHelper<ExponentialMovingAverage>::ExecutorMap),
      alpha(_alpha) {}

  ~EMAFilter() {}

  Try<Nothing> consume(const ResourceUsage& in);

  virtual Try<Nothing> filter(
      ExponentialMovingAverage* ema,
      const ResourceUsage_Executor& previousExec,
      const ResourceUsage_Executor& currentExec,
      ResourceUsage_Executor* outExec) = 0;

 protected:
  double_t alpha;
  std::unique_ptr<ExecutorSet> previousSamples;
  std::unique_ptr<MapHelper<ExponentialMovingAverage>::ExecutorMap> emaSamples;
};


/*
 * IpcEMAFilter calculates Exponential Moving Average of IPC.
 * It gets IPC from perf statistics in ResourceStatistics.
 * It stores the calculated value in statistics.net_tcp_active_connections
 * field.
 * NOTE: It requires perf enabled on slave node.
 */
class IpcEMAFilter : public EMAFilter {
 public:
  IpcEMAFilter(Consumer<ResourceUsage>* _consumer, double_t _alpha) :
      EMAFilter(_consumer, _alpha) {}

  ~IpcEMAFilter();

  Try<Nothing> filter(
      ExponentialMovingAverage* ema,
      const ResourceUsage_Executor& previousExec,
      const ResourceUsage_Executor& currentExec,
      ResourceUsage_Executor* outExec);
};


 /*
 * CpuUsageEMAFilter calculates Exponential Moving Average of cpu usage.
 * It gets CpuUsage from statistics in ResourceUsage_Executor.
 * It stores the calculated value in statistics.net_tcp_time_wait_connections
 * field.
 */
class CpuUsageEMAFilter : public EMAFilter {
 public:
  CpuUsageEMAFilter(
      Consumer<ResourceUsage>* _consumer, double_t _alpha)
      : EMAFilter(_consumer, _alpha) { }

  ~CpuUsageEMAFilter();

  Try<Nothing> filter(
      ExponentialMovingAverage* ema,
      const ResourceUsage_Executor& previousExec,
      const ResourceUsage_Executor& currentExec,
      ResourceUsage_Executor* outExec);
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_EXPONENTIAL_MOVING_AVERAGE_FILTER_HPP
