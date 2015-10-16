#include <utility>

#include "detectors/assurance_fractional.hpp"
#include "detectors/detector.hpp"

#include "filters/drop.hpp"

#include "messages/serenity.hpp"

#include "stout/none.hpp"

namespace mesos {
namespace serenity {

Result<ChangePointDetection> AssuranceFractionalDetector::processSample(
    double_t in) {
  this->window.push_back(in);

  if (this->window.size() < this->state.windowSize) {
    return None();  // Only warm up.
  }

  double_t basePoint = this->window.front();
  this->window.pop_front();

  if (this->referencePoint.isSome()) {
    // Check if the signal returned to normal state. (!)
    double_t nearValue = this->state.nearFraction * this->referencePoint.get();
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
  << " | threshold %: " << this->state.fractionalThreshold * 100;

  // If drop fraction is higher than threshold or signal did not returned
  // after cooldown, then trigger contention.
  if (currentDropFraction > this->state.fractionalThreshold ||
      (this->referencePoint.isSome())) {
    ChangePointDetection cpd;
    if (this->referencePoint.isNone()) {
      cpd.severity =  currentDropFraction * this->state.severityLevel;
      this->lastSeverity = cpd.severity;
    } else {
      cpd.severity =  this->lastSeverity;
    }

    LOG(INFO) << tag.NAME() << " Created contention with severity = "
              << cpd.severity;

    this->contentionCooldownCounter = this->state.contentionCooldown;
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

// Fix for using templated methods in .cpp file.
// See:
//    https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template class DropFilter<AssuranceFractionalDetector>;

}  // namespace serenity
}  // namespace mesos
