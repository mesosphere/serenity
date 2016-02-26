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
      const Config& _conf,
      const Tag& _tag = Tag(QOS_CONTROLLER, NAME))
      : Producer<ResourceUsage>(_consumer), tag(_tag) {
    setMinimalCpuUsage(_conf.getValue<double_t>(MINIMAL_CPU_USAGE_KEY));
  }

  ~TooLowUsageFilter();



  Try<Nothing> consume(const ResourceUsage& in);

  void setMinimalCpuUsage(Result<double_t> cfgMinimalCpuUsage) {
    cfgMinimalCpuUsage =
      ConfigValidator<double_t>(cfgMinimalCpuUsage, MINIMAL_CPU_USAGE_KEY)
        .validateValueIsPositive()
        .getOrElse(MINIMAL_CPU_USAGE_DEFAULT);
  }

  static const constexpr char* NAME = "TooLowUsageFilter";

  static const constexpr char* MINIMAL_CPU_USAGE_KEY = "MINIMAL_CPU_USAGE";

 protected:
  const Tag tag;

  double_t cfgMinimalCpuUsage;

  static constexpr double_t MINIMAL_CPU_USAGE_DEFAULT = 0.25;  // !< per sec.
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_TOO_LOW_USAGE_FILTER_HPP
