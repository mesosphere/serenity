#ifndef SERENITY_BASE_DETECTOR_HPP
#define SERENITY_BASE_DETECTOR_HPP

#include <memory>
#include <string>

#include "serenity/config.hpp"
#include "serenity/serenity.hpp"

#include "stout/nothing.hpp"
#include "stout/try.hpp"
#include "stout/result.hpp"

namespace mesos {
namespace serenity {

struct Detection {
  double_t severity;
};


/**
 * Sequential point detection interface.
 * It can receive and process observations sequentially over time.
 */
class BaseDetector {
 public:
  explicit BaseDetector(const Tag& _tag,
                        const SerenityConfig _config)
    : tag(_tag), cfg(_config) {
  }

  static std::shared_ptr<BaseDetector> makeDetector(std::string);

  virtual Result<Detection> processSample(double_t in) { return None(); }

 protected:
  const Tag tag;
  SerenityConfig cfg;
  uint64_t contentionCooldownCounter = 0;
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_BASE_DETECTOR_HPP
