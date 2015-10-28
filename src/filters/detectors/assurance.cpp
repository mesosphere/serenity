#include <utility>

#include "filters/detectors/assurance.hpp"

#include "messages/serenity.hpp"

#include "stout/none.hpp"

namespace mesos {
namespace serenity {

Result<Detection> AssuranceDetector::processSample(
    double_t in) {
  this->window.push_back(in);

  if (this->window.size() < this->cfg.getU64(detector::WINDOW_SIZE)) {
    return None();  // Only warm up.
  }

  double_t basePoint = this->window.front();
  this->window.pop_front();

  if (this->referencePoint.isSome()) {
    // Check if the signal returned to normal state. (!)
    double_t nearValue =
      this->cfg.getD(detector::NEAR_FRACTION) * this->referencePoint.get();
    SERENITY_LOG(INFO) << "Waiting for signal to return to "
                       << (this->referencePoint.get() - nearValue)
                       << " (base value) after "
                       << "corrections. Waiting iteration: "
                       << referencePointCounter;
    this->referencePointCounter++;
    // We want to use reference Base Point instead of base point.
    basePoint = in;
    if (in >= (this->referencePoint.get() - nearValue)) {
      this->referencePoint = None();
      SERENITY_LOG(INFO) << "Signal returned to established state.";
    }
  }

  if (this->contentionCooldownCounter > 0) {
    SERENITY_LOG(INFO) << "Cooldown for: " << contentionCooldownCounter;
    this->contentionCooldownCounter--;
    return None();
  }

  // Current drop fraction indicates how much value has drop in relation to
  // base point
  double_t currentDropFraction = 1.0 - (in / basePoint);

  SERENITY_LOG(INFO)
  << " inputValue: " << in
  << " | baseValue = " << basePoint
  << " | current drop %: " << currentDropFraction * 100
  << " | threshold %: "
  << this->cfg.getD(detector::FRACTIONAL_THRESHOLD) * 100;

  // If drop fraction is higher than threshold or signal did not returned
  // after cooldown, then trigger contention.
  if (currentDropFraction > this->cfg.getD(detector::FRACTIONAL_THRESHOLD) ||
      (this->referencePoint.isSome())) {
    Detection cpd;
    if (this->referencePoint.isNone()) {
      cpd.severity =
        currentDropFraction * this->cfg.getD(detector::SEVERITY_FRACTION);
      this->lastSeverity = cpd.severity;
    } else {
      cpd.severity =  this->lastSeverity;
    }

    LOG(INFO) << tag.NAME() << " Created contention with severity = "
              << cpd.severity;

    this->contentionCooldownCounter =
      this->cfg.getU64(detector::CONTENTION_COOLDOWN);
    this->referencePoint = basePoint;
    this->referencePointCounter = 0;

    return cpd;
  }

  if (in < basePoint) {
    SERENITY_LOG(INFO) << " Found decrease, "
        "but not significant: " << (currentDropFraction * 100) << "%";
  }

  return None();
}

}  // namespace serenity
}  // namespace mesos
