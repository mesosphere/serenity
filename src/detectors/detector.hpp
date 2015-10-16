#ifndef SERENITY_DROP_DETECTOR_HPP
#define SERENITY_DROP_DETECTOR_HPP

#include "serenity/config.hpp"
#include "serenity/serenity.hpp"

#include "stout/nothing.hpp"
#include "stout/try.hpp"
#include "stout/result.hpp"

namespace mesos {
namespace serenity {

struct ChangePointDetection {
  double_t severity;
};


/**
 * Sequential change point detection interface.
 * It can receive and process observations sequentially over time.
 */
class ChangePointDetector {
 public:
  explicit ChangePointDetector(const Tag& _tag): tag(_tag) {}

  virtual Try<Nothing> configure(ChangePointDetectionState cpdState) {
    this->state = cpdState;

    return Nothing();
  }

  virtual Result<ChangePointDetection> processSample(double_t in) = 0;

 protected:
  const Tag tag;
  ChangePointDetectionState state;
  uint64_t contentionCooldownCounter = 0;
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_DROP_DETECTOR_HPP
