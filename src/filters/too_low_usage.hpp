#ifndef SERENITY_TOO_LOW_USAGE_FILTER_HPP
#define SERENITY_TOO_LOW_USAGE_FILTER_HPP

#include <ctime>
#include <list>
#include <memory>
#include <string>

#include "mesos/mesos.hpp"

#include "serenity/config.hpp"
#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

/**
 * Filter out PR executors with too low metrics.
 * Currently we filter out when CPU Usage is below specified threshold.
 */
class TooLowUsageFilter :
    public Consumer<ResourceUsage>, public Producer<ResourceUsage> {
 public:
  explicit TooLowUsageFilter(const Tag& _tag = Tag(QOS_CONTROLLER, NAME))
    : tag(_tag) {}

  explicit TooLowUsageFilter(
      Consumer<ResourceUsage>* _consumer,
      const SerenityConfig& _conf,
      const Tag& _tag = Tag(QOS_CONTROLLER, NAME))
      : Producer<ResourceUsage>(_consumer), tag(_tag) {
    setCfgMinimalCpuUsage(_conf.getItemOrDefault<double_t>(
        too_low_usage::MINIMAL_CPU_USAGE,
        too_low_usage::DEFAULT_MINIMAL_CPU_USAGE));
  }

  ~TooLowUsageFilter();

  static const constexpr char* NAME = "TooLowUsageFilter";

  Try<Nothing> consume(const ResourceUsage& in);

  void setCfgMinimalCpuUsage(double_t cfgMinimalCpuUsage) {
    TooLowUsageFilter::cfgMinimalCpuUsage = cfgMinimalCpuUsage;
  }

 protected:
  const Tag tag;

  double_t cfgMinimalCpuUsage;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_TOO_LOW_USAGE_FILTER_HPP
