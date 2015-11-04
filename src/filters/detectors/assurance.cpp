#include <list>
#include <utility>

#include "filters/detectors/assurance.hpp"

#include "messages/serenity.hpp"

#include "stout/none.hpp"

namespace mesos {
namespace serenity {

void AssuranceDetector::shiftBasePoints() {
  for (std::list<double_t>::iterator& basePoint : this->basePoints) {
    basePoint++;
  }
}

Detection AssuranceDetector::createContention(double_t severity) {
  Detection cpd;
  cpd.severity = severity;
  SERENITY_LOG(INFO) << " Created contention with severity = "
                      << cpd.severity;
}

Result<Detection> AssuranceDetector::processSample(double_t in) {
  // Fill window.
  if (in < 0.1)
    in = 0.1;
  this->window.push_back(in);

  Result<Detection> result = this->_processSample(in);

  // Always at the end of sample process.
  this->shiftBasePoints();
  this->window.pop_front();

  return result;
}


Try<Nothing> AssuranceDetector::reset() {
  // Return detector to normal state.
  SERENITY_LOG(INFO) << "Resetting.";
  this->valueBeforeDrop = None();

  return Nothing();
}


Result<Detection> AssuranceDetector::_processSample(
    double_t in) {

  if (this->valueBeforeDrop.isSome()) {
    // Check if the signal returned to normal state. (!)
    double_t nearValue =
      this->cfg.getD(detector::NEAR_FRACTION) * this->valueBeforeDrop.get();
    SERENITY_LOG(INFO) << "Waiting for signal: "
                       << in << " to return to: "
                       << (this->valueBeforeDrop.get() - nearValue)
                       << "after corrections. ";
    // We want to use reference Base Point instead of base point.
    if (in >= (this->valueBeforeDrop.get() - nearValue)) {
      this->reset();
      SERENITY_LOG(INFO) << "Signal returned to established state.";
    } else {
      // Create contention.
      return this->createContention(
        1 * this->cfg.getD(detector::SEVERITY_FRACTION));
    }
  }

  double_t currentDropFraction = 0;
  double_t meanValueBeforeDrop = 0;
  this->dropVotes = 0;
  std::stringstream basePointValues;

  for (std::list<double_t>::iterator basePoint : this->basePoints) {
    basePointValues << " " << (double_t)(*basePoint);
    // Check if drop happened for this basePoint.
    double_t dropFraction = 1.0 - (in / (*basePoint));
    if (dropFraction >=
        this->cfg.getD(detector::FRACTIONAL_THRESHOLD)) {
      // Vote on drop.
      this->dropVotes++;
      currentDropFraction += dropFraction;
      meanValueBeforeDrop += (double_t)(*basePoint);
      basePointValues << "[-] ";
    } else {
      basePointValues << "[+] ";
    }
  }

  if (this->dropVotes > 0) {
    currentDropFraction /= this->dropVotes;
    meanValueBeforeDrop /= this->dropVotes;
  }  // In other cases theses variables == 0.

  SERENITY_LOG(INFO)
  << "{inValue: " << in
  << " |baseValues:" << basePointValues.str()
  << " |currentDrop %: " << currentDropFraction * 100
  << " |threshold %: "
  << this->cfg.getD(detector::FRACTIONAL_THRESHOLD) * 100
  << " |dropVotes/quorum: " << this->dropVotes
  << "/" << this->cfg.getU64(detector::QUORUM)
  << "}";


  // Check if drop obtained minimum number of votes.
  if (this->dropVotes >= this->cfg.getU64(detector::QUORUM)) {
    // Create contention.
    this->valueBeforeDrop = meanValueBeforeDrop;
    return this->createContention(
      currentDropFraction * this->cfg.getD(detector::SEVERITY_FRACTION));
  }

  return None();
}

}  // namespace serenity
}  // namespace mesos
