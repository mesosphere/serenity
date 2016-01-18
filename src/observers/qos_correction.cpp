#include <list>
#include <vector>
#include <utility>

#include "bus/event_bus.hpp"

#include "observers/qos_correction.hpp"

#include "serenity/resource_helper.hpp"

namespace mesos {
namespace serenity {

QoSCorrectionObserver::~QoSCorrectionObserver() {}

Try<Nothing> QoSCorrectionObserver::qosCorrectionStateMachine() {
  if (contentions.get().empty() ||
      ResourceUsageHelper::getRevocableExecutors(usage.get()).empty()) {
    noContentionsReceived();
    // Produce empty corrections and contentions
    produceResultsAndClearConsumedData();
    return Nothing();
  }

  if (iterationCooldownCounter.isSome()) {
    cooldownPhase();
    // Produce empty corrections and contentions
    produceResultsAndClearConsumedData();
    return Nothing();
  }

  Try<QoSCorrections> corrections = newContentionReceived();
  if (corrections.isError()) {
    // Produce empty corrections and contentions
    produceResultsAndClearConsumedData();
    return Error(corrections.error());
  }

  if (corrections.get().empty()) {
    // Strategy didn't found aggressors.
    // Passing contentions to next QoS Controller.
    produceResultsAndClearConsumedData(QoSCorrections(),
                                       this->contentions.get());
    return Nothing();
  }

  // Strategy has pointed aggressors, so don't pass
  // current contentions to next QoS Controller.
  produceResultsAndClearConsumedData(corrections.get(),
                                     Contentions());
  return Nothing();
}

void QoSCorrectionObserver::produceResultsAndClearConsumedData(
    QoSCorrections _qosCorrections,
    Contentions _contentions) {
  produce<QoSCorrections>(_qosCorrections);
  produce<Contentions>(_contentions);

  this->contentions = None();
  this->usage = None();
}

void QoSCorrectionObserver::noContentionsReceived() {
  // Restart state of QoSCorrection observer
  if (iterationCooldownCounter.isSome()) {
    iterationCooldownCounter = None();
  }
  if (estimatorPipelineDisabled) {
    StaticEventBus::publishOversubscriptionCtrlEvent(true);
    estimatorPipelineDisabled = false;
  }
}

void QoSCorrectionObserver::cooldownPhase() {
  // Cooldown phase - waiting for system to stabilize.
  iterationCooldownCounter.get() -= 1;
  if (iterationCooldownCounter.get() <= 0) {
    iterationCooldownCounter = None();
  }
  return;
}

Try<QoSCorrections> QoSCorrectionObserver::newContentionReceived() {
  iterationCooldownCounter = this->cooldownIterations;
  if (!estimatorPipelineDisabled) {
    // Disable Estimator pipeline.
    StaticEventBus::publishOversubscriptionCtrlEvent(false);
    estimatorPipelineDisabled = true;
  }
  return this->revocationStrategy->decide(this->executorAgeFilter,
                                          this->contentions.get(),
                                          this->usage.get());
}

Try<Nothing> QoSCorrectionObserver::syncConsume(
    const std::vector<Contentions> products) {
  this->contentions = Contentions();
  for (Contentions contentions : products) {
    for (Contention contention : contentions) {
      this->contentions.get().push_back(contention);
    }
  }

  if (isAllDataForCorrectionGathered()) {
    this->qosCorrectionStateMachine();
  }

  return Nothing();
}

Try<Nothing> QoSCorrectionObserver::consume(const ResourceUsage& usage) {
  this->usage = usage;

  if (isAllDataForCorrectionGathered()) {
    this->qosCorrectionStateMachine();
  }

  return Nothing();
}

bool QoSCorrectionObserver::isAllDataForCorrectionGathered() {
  return this->contentions.isSome() && this->usage.isSome();
}

}  // namespace serenity
}  // namespace mesos
