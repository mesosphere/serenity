#ifndef SERENITY_SIGNAL_ANALYZER_HPP
#define SERENITY_SIGNAL_ANALYZER_HPP

#include <memory>
#include <string>

#include "serenity/serenity.hpp"

#include "stout/nothing.hpp"
#include "stout/try.hpp"
#include "stout/result.hpp"

namespace mesos {
namespace serenity {

struct Detection {
  Detection() : severity(None()) {}

  Option<double_t> severity;
};

/**
 * Sequential signal analyzer interface.
 * It can receive and process observations sequentially over time.
 */
class SignalAnalyzer {
 public:
  explicit SignalAnalyzer(const Tag& _tag) : tag(_tag) {
  }

  virtual Result<Detection> processSample(double_t in) = 0;

  virtual Try<Nothing> resetSignalRecovering() = 0;

 protected:
  const Tag tag;

  /**
   * Contention Factory.
   */
  Detection createContention(double_t severity) {
    Detection cpd;
    if (severity > 0) {
      cpd.severity = severity;
    }

    SERENITY_LOG(INFO) << " Created contention with severity = "
    << (cpd.severity.isSome() ? std::to_string(cpd.severity.get()) : "<none>");
    return cpd;
  }
};


}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_SIGNAL_ANALYZER_HPP
