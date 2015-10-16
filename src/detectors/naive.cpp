#include <utility>

#include "detectors/detector.hpp"
#include "detectors/naive.hpp"

#include "filters/drop.hpp"

#include "messages/serenity.hpp"

#include "stout/none.hpp"

namespace mesos {
namespace serenity {

Result<ChangePointDetection> NaiveChangePointDetector::processSample(
    double_t in) {
  if (this->contentionCooldownCounter > 0) {
    this->contentionCooldownCounter--;

    return None();
  }

  if (in < this->state.absoluteThreshold) {
    this->contentionCooldownCounter = this->state.contentionCooldown;
    ChangePointDetection cpd;
    cpd.severity = this->state.absoluteThreshold - in;
    return cpd;
  }

  return None();
}


// Fix for using templated methods in .cpp file.
// See:
//    https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template class DropFilter<NaiveChangePointDetector>;

}  // namespace serenity
}  // namespace mesos
