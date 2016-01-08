#ifndef SERENITY_QOS_CORRECTION_HPP
#define SERENITY_QOS_CORRECTION_HPP

#include <list>
#include <string>
#include <vector>

#include "filters/executor_age.hpp"

#include "glog/logging.h"

#include "mesos/mesos.hpp"
#include "mesos/resources.hpp"

#include "mesos/slave/oversubscription.hpp"

#include "messages/serenity.hpp"

#include "serenity/config.hpp"
#include "serenity/serenity.hpp"

#include "observers/strategies/base.hpp"
#include "observers/strategies/seniority.hpp"

namespace mesos {
namespace serenity {

/**
 * QoSCorrectionObserver observes incoming Contentions and
 * ResourceUsage and produces QoSCorrections.
 *
 * Currently Mesos Agent understand only kill correction action.
 */
class QoSCorrectionObserver : public SyncConsumer<Contentions>,
                              public Consumer<ResourceUsage>,
                              public Producer<QoSCorrections> {
 public:
  explicit QoSCorrectionObserver(
      Consumer<QoSCorrections>* _consumer,
      uint64_t _contentionProducents,
      const SerenityConfig& _config,
      ExecutorAgeFilter* _ageFilter = new ExecutorAgeFilter(),
      RevocationStrategy* _revStrategy = nullptr)
    : SyncConsumer<Contentions>(_contentionProducents),
      Producer<QoSCorrections>(_consumer),
      currentContentions(None()),
      currentUsage(None()),
      revStrategy(_revStrategy),
      ageFilter(_ageFilter),
      config(_config) {

    if (revStrategy == nullptr) {
      revStrategy = new SeniorityStrategy(this->config);
    }
  }

  ~QoSCorrectionObserver();

  Try<Nothing> _syncConsume(const std::vector<Contentions> products) override;

  Try<Nothing> consume(const ResourceUsage& usage) override;

  static bool compareSeverity(
      const Contention& first, const Contention& second) {
    if (!second.has_severity())
      return true;
    if (!first.has_severity())
      return false;

    return (first.severity() > second.severity());
  }

  static bool compareCpuAllocated(
      const ResourceUsage_Executor& first,
      const ResourceUsage_Executor& second) {
    // TODO(bplotka) decide if we want to validate fields here.
    Resources secondAllocation(second.allocated());
    if (secondAllocation.cpus().isNone())
      return true;

    Resources firstAllocation(first.allocated());
    if (firstAllocation.cpus().isNone())
      return false;

    return (firstAllocation.cpus().get() > secondAllocation.cpus().get());
  }

  static constexpr const char* NAME = "CorrectionObserver: ";
  static constexpr const char* name = "[SerenityQoS] CorrectionObserver: ";

 protected:
  Option<Contentions> currentContentions;
  Option<ResourceUsage> currentUsage;
  RevocationStrategy* revStrategy;
  ExecutorAgeFilter* ageFilter;

  const SerenityConfig config;

  //! Run when all required info are gathered.
  Try<Nothing> correctAgent();
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_QOS_CORRECTION_HPP
