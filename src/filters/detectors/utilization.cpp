#include <list>
#include <utility>

#include "filters/detectors/utilization.hpp"

#include "messages/serenity.hpp"

#include "stout/none.hpp"

namespace mesos {
namespace serenity {


Result<Detection> UtilizationDetector::processSample(double_t in) {
  // TODO(bplotka): Implement this.
}


Try<Nothing> UtilizationDetector::reset() {
  // Return detector to normal state.
  SERENITY_LOG(INFO) << "Resetting any drop tracking if exists.";
  return Nothing();
}


}  // namespace serenity
}  // namespace mesos
