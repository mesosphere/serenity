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
class QoSCorrectionObserver : public Consumer<Contentions>,
                              public Consumer<ResourceUsage>,
                              public Producer<QoSCorrections>,
                              public Producer<Contentions> {
 public:
  explicit QoSCorrectionObserver(
      Consumer<QoSCorrections>* _consumer,
      ExecutorAgeFilter* _ageFilter = new ExecutorAgeFilter(),
      RevocationStrategy* _revStrategy =
          new SeniorityStrategy(Config()),
      int64_t _cooldownIterations = CONTENTION_COOLDOWN_DEFAULT,
      const Tag& _tag = Tag(QOS_CONTROLLER, NAME)) :
        Producer<QoSCorrections>(_consumer),
        revocationStrategy(_revStrategy),
        executorAgeFilter(_ageFilter),
        cfgCooldownIterations(_cooldownIterations),
        tag(_tag) {}

  ~QoSCorrectionObserver();

  template <typename T>
  Try<Nothing> addConsumer(T* consumer) {
    return Producer<T>::addConsumer(consumer);
  }

  static constexpr const char* NAME = "QoSCorrectionObserver";

  static const constexpr char* CONTENTION_COOLDOWN_KEY = "CONTENTION_COOLDOWN";

 protected:
  void allProductsReady() override;

  void emptyContentionsReceived();

  Try<QoSCorrections> newContentionsReceived();

  void cooldownPhase();

  void produceResults(QoSCorrections _corrections = QoSCorrections(),
                      Contentions _contentions = Contentions());

  RevocationStrategy* revocationStrategy;

  ExecutorAgeFilter* executorAgeFilter;

  const Tag tag;

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
  int64_t cfgCooldownIterations;

  static constexpr int64_t CONTENTION_COOLDOWN_DEFAULT = 10;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_QOS_CORRECTION_HPP
