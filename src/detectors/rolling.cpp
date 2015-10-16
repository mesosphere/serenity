#include <utility>

#include "detectors/detector.hpp"
#include "detectors/rolling.hpp"

#include "filters/drop.hpp"

#include "messages/serenity.hpp"

#include "stout/none.hpp"

namespace mesos {
namespace serenity {


Result<ChangePointDetection> RollingChangePointDetector::processSample(
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

  if (in < (basePoint - this->state.relativeThreshold)) {
    this->contentionCooldownCounter = this->state.contentionCooldown;
    ChangePointDetection cpd;

    // We calculate severity as difference between threshold and actual
    // processed value.
    cpd.severity = (basePoint - this->state.relativeThreshold) - in;
    return cpd;
  }

  if (in < basePoint) {
    SERENITY_LOG(INFO) << " Found decrease, "
                 "but not significant: " << (in - basePoint);
  }

  return None();
}

// Fix for using templated methods in .cpp file.
// See:
//    https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template class DropFilter<RollingChangePointDetector>;

}  // namespace serenity
}  // namespace mesos
