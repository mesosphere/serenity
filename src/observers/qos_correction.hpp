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
 * TODO(skonefal): create proper documentation from this rough draft.
 *
 * QoSCorrection consumes Contentions and passes them to Strategy
 * to produce revocations.
 *
 * When new contention appears, it sends "Disable" message to
 * Estimator pipeline to make environment stable.
 *
 * When strategy generates QoSCorrections, OoSCorrectionObserver starts
 * CooldownCounter, to give platform time to stabilize.
 * During cooldown, QoSCorrectionObserved does not produce new Corrections.
 *
 * When strategy does not yield any Corrections, QoSCorrectionObserver
 * passes Contentions to next QoSCorrectionObserver in pipeline (and,
 * produces empty Corrections).
 */
class QoSCorrectionObserver : public SyncConsumer<Contentions>,
                              public Consumer<ResourceUsage>,
                              public Producer<QoSCorrections>,
                              public Producer<Contentions> {
 public:
  explicit QoSCorrectionObserver(
      Consumer<QoSCorrections>* _consumer,
      uint64_t _contentionProducents,
      ExecutorAgeFilter* _ageFilter = new ExecutorAgeFilter(),
      RevocationStrategy* _revStrategy =
          new SeniorityStrategy(SerenityConfig()),
      uint32_t _cooldownIterations = strategy::DEFAULT_CONTENTION_COOLDOWN,
      const Tag& _tag = Tag(QOS_CONTROLLER, NAME))
    : SyncConsumer<Contentions>(_contentionProducents),
      Producer<QoSCorrections>(_consumer),
      contentions(None()),
      usage(None()),
      revocationStrategy(_revStrategy),
      executorAgeFilter(_ageFilter),
      cooldownIterations(_cooldownIterations),
      tag(_tag) {}

  ~QoSCorrectionObserver();

  Try<Nothing> syncConsume(const std::vector<Contentions> products) override;
  Try<Nothing> consume(const ResourceUsage& usage) override;

  template <typename T>
  Try<Nothing> addConsumer(T* consumer) {
    return Producer<T>::addConsumer(consumer);
  }

  static constexpr const char* NAME = "QoSCorrectionObserver";

 protected:
  Try<Nothing> doQosDecision();

  void emptyContentionsReceived();

  Try<QoSCorrections> newContentionsReceived();

  void cooldownPhase();

  bool isAllDataForCorrectionGathered();

  void produceResultsAndClearConsumedData(
                        QoSCorrections _corrections = QoSCorrections(),
                        Contentions _contentions = Contentions());

  RevocationStrategy* revocationStrategy;

  ExecutorAgeFilter* executorAgeFilter;

  const Tag tag;

  Option<Contentions> contentions;  //!< Contention resources from other filters
  Option<ResourceUsage> usage;  //!< Resource usage from other filters

  template <typename T>
  Try<Nothing> produce(T product) {
    return Producer<T>::produce(product);
  }

  /**
   *  QoSCorrectionObserver correction observer sends 'disable'
   *  message to valve in Estimator pipeline when contention arises.
   *
   *  If this happens, this variable is 'TRUE'
   */
  bool estimatorPipelineDisabled;

  /**
   * Counter (in iterations) until next revocation.
   * It used to create some time for system to stabilize.
   */
  Option<uint64_t> iterationCooldownCounter;

  // Configuration parameters.

  /**
   * Default iterationCooldownCounter start value.
   */
  uint64_t cooldownIterations;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_QOS_CORRECTION_HPP
