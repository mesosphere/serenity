#include <utility>

#include "detectors/detector.hpp"
#include "detectors/rolling_fractional.hpp"

#include "filters/drop.hpp"

#include "messages/serenity.hpp"

#include "stout/none.hpp"

namespace mesos {
namespace serenity {


Result<ChangePointDetection> RollingFractionalDetector::processSample(
    double_t in) {
  this->window.push_back(in);

  if (this->window.size() < this->state.windowSize) {
    return None();  // Only warm up.
  }

  double_t basePoint = this->window.front();
  this->window.pop_front();

  if (this->contentionCooldownCounter > 0) {
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

  // If drop fraction is higher than threshold, then trigger contention.
  if (currentDropFraction > this->state.fractionalThreshold) {
    this->contentionCooldownCounter = this->state.contentionCooldown;
    ChangePointDetection cpd;

    cpd.severity = currentDropFraction * this->state.severityLevel;
    LOG(INFO) << tag.NAME() << " Created contention with severity = "
              << cpd.severity;
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
template class DropFilter<RollingFractionalDetector>;

}  // namespace serenity
}  // namespace mesos
