#ifndef SERENITY_CORRECTION_MERGER_FILTER_HPP
#define SERENITY_CORRECTION_MERGER_FILTER_HPP

#include <algorithm>
#include <ctime>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "mesos/mesos.hpp"

#include "messages/serenity.hpp"

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

/**
 * Merges several observer's corrections into one. Checks for duplicates.
 */
class CorrectionMergerFilter:
  public Consumer<QoSCorrections>, public Producer<QoSCorrections> {
 public:
  explicit CorrectionMergerFilter(
    Consumer<QoSCorrections>* _consumer,
    const Tag& _tag = Tag(QOS_CONTROLLER, NAME))
    : Producer<QoSCorrections>(_consumer),
      tag(_tag) {}

  ~CorrectionMergerFilter() {}

  virtual void allProductsReady() {
    std::vector<QoSCorrections> qosCorrectionsVector =
        Consumer<QoSCorrections>::getConsumables();
    QoSCorrections corrections;

    uint64_t receivedCententionNum = 0;
    for (QoSCorrections product : qosCorrectionsVector) {
      receivedCententionNum += product.size();
      for (slave::QoSCorrection correction : product) {
        if (checkForDuplicates(correction, corrections)) {
          // Filter out duplicated value.
          continue;
        }
        corrections.push_back(correction);
      }
    }

    SERENITY_LOG(INFO) << "Received " << corrections.size() << " corrections";
    produce(corrections);
    return;
  }

 private:
  const Tag tag;

  static const constexpr char* NAME = "CorrectionMerger";

  // Returns True when value is duplicated in list.
  // TODO(bplotka): Move to QoSCorrections std::set in future.
  bool checkForDuplicates(
      slave::QoSCorrection value, QoSCorrections corrections) {
    for (slave::QoSCorrection correction : corrections) {
      if (value.type() == slave::QoSCorrection_Type_KILL &&
          value.has_kill() &&
          value.kill().has_executor_id() &&
          value.kill().has_framework_id() &&
          correction.type() == slave::QoSCorrection_Type_KILL &&
          correction.has_kill() &&
          correction.kill().has_executor_id() &&
          correction.kill().has_framework_id()) {
        if (correction.kill().executor_id().value() ==
            value.kill().executor_id().value() &&
            correction.kill().framework_id().value() ==
            value.kill().framework_id().value()) {
          // Found duplicate.
          return true;
        }
      } else {
        SERENITY_LOG(WARNING)
          << "Received correction without all required data.";
      }
    }
    return false;
  }
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_CORRECTION_MERGER_FILTER_HPP
