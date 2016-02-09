#include <list>
#include <vector>
#include <utility>

#include "bus/event_bus.hpp"

#include "observers/qos_correction.hpp"

#include "serenity/resource_helper.hpp"
#include "serenity/utils.hpp"

namespace mesos {
namespace serenity {

QoSCorrectionObserver::~QoSCorrectionObserver() {}

void QoSCorrectionObserver::allProductsReady() {
  Contentions contentions = flattenListsInsideVector<Contentions>(
      Consumer<Contentions>::getConsumables());
  Option<ResourceUsage> usage = Consumer<ResourceUsage>::getConsumable();

  if (contentions.size() == 0  ||
      ResourceUsageHelper::getRevocableExecutors(usage.get()).empty()) {
    SERENITY_LOG(INFO) << "Empty contentions received.";
    emptyContentionsReceived();

    // Produce empty corrections and contentions
    produceResults();
    return;
  }

  // We have contentions, but we are in "stablisation" phase.
  if (iterationCooldownCounter.isSome()) {
    SERENITY_LOG(INFO) << "QoS Correction observer is in cooldown phase";
    cooldownPhase();

    // Produce empty corrections and contentions
    produceResults();
    return;
  }

  Try<QoSCorrections> corrections = newContentionsReceived();
  if (corrections.isError()) {
    SERENITY_LOG(INFO) << "corrections returned error: " << corrections.error();
    // Produce empty corrections and contentions
    produceResults();
    return;
  }

  if (corrections.get().empty()) {
    SERENITY_LOG(INFO) << "Strategy didn't found aggressors";
    // Strategy didn't found aggressors.
    // Passing contentions to next QoS Controller.
    produceResults(QoSCorrections(), contentions);
    return;
  }

  // Strategy has pointed aggressors, so don't pass
  // current contentions to next QoS Controller.
  iterationCooldownCounter = this->cooldownIterations;
  produceResults(corrections.get(), Contentions());
}

void QoSCorrectionObserver::produceResults(
    QoSCorrections _qosCorrections,
    Contentions _contentions) {
  produce<QoSCorrections>(_qosCorrections);
  produce<Contentions>(_contentions);
}

void QoSCorrectionObserver::emptyContentionsReceived() {
  // Restart state of QoSCorrection observer
  if (iterationCooldownCounter.isSome()) {
    iterationCooldownCounter = None();
  }
  if (estimatorPipelineDisabled) {
    StaticEventBus::publishOversubscriptionCtrlEvent(true);
    estimatorPipelineDisabled = false;
  }
}

/**
 * Cooldown phase - waiting for system to stabilise.
 */
void QoSCorrectionObserver::cooldownPhase() {
  if (iterationCooldownCounter.isNone()) {
    return;
  }

  iterationCooldownCounter.get() -= 1;
  if (iterationCooldownCounter.get() <= 0) {
    iterationCooldownCounter = None();
  }
  return;
}

Try<QoSCorrections> QoSCorrectionObserver::newContentionsReceived() {
  if (!estimatorPipelineDisabled) {
    // Disable Estimator pipeline.
    StaticEventBus::publishOversubscriptionCtrlEvent(false);
    estimatorPipelineDisabled = true;
  }

  Contentions contentions = flattenListsInsideVector<Contentions>(
      Consumer<Contentions>::getConsumables());
  Option<ResourceUsage> usage = Consumer<ResourceUsage>::getConsumable();
  return this->revocationStrategy->decide(this->executorAgeFilter,
                                          contentions,
                                          usage.get());
}

}  // namespace serenity
}  // namespace mesos
