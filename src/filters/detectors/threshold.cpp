#include <list>
#include <utility>

#include "filters/detectors/threshold.hpp"

#include "messages/serenity.hpp"

#include "stout/none.hpp"

namespace mesos {
namespace serenity {


Result<Detection> ThresholdDetector::processSample(double_t in) {
  if (in > this->cfgThreshold) {
    SERENITY_LOG(INFO) << "Value above threshold. Remove all from host. "
        "Value: " << in << " Threshold: " << this->cfgThreshold;
    return this->createContention(KILL_ALL_SEVERITY);
  }

  return None();
}


Try<Nothing> ThresholdDetector::reset() {
  // Return detector to normal state.
  SERENITY_LOG(INFO) << "Resetting any drop tracking if exists.";
  return Nothing();
}


}  // namespace serenity
}  // namespace mesos
