#include <list>
#include <utility>

#include "contention_detectors/signal_analyzers/drop.hpp"

#include "messages/serenity.hpp"

#include "stout/none.hpp"

namespace mesos {
namespace serenity {

void SignalDropAnalyzer::shiftBasePoints() {
  for (std::list<double_t>::iterator& basePoint : this->basePoints) {
    basePoint++;
  }
}


// In case of parameters modification we need to recalculate internal state.
void SignalDropAnalyzer::recalculateParams() {
  this->window.clear();
  this->basePoints.clear();

  uint64_t windowSize = this->cfgWindowSize;

  // Find the biggest n in the T-2^n which fits within window length.
  uint64_t checkpoints = 0;
  while (windowSize > 0) {
    windowSize = windowSize >> 1;
    checkpoints++;
  }

  // Make sure it does not exceed MAX_CHECKPOINSs option.
  if (checkpoints > this->cfgMaxCheckpoints) {
    checkpoints = this->cfgMaxCheckpoints;
  }

  // Get the Quorum number from QUORUM fraction parameter.
  this->quorumNum = this->cfgQuroum * checkpoints;
  if (this->quorumNum == 0 || this->quorumNum > checkpoints) {
    SERENITY_LOG(WARNING) << "Bad value for Quorum parameter. Creating 100%"
                          << " quorum.";
    this->quorumNum = checkpoints;
  }

  std::stringstream checkpointLog;
  checkpointLog << "Assurance Parameters: Quorum = "
                << this->quorumNum << "/"
                << checkpoints << " Checkpoints [ ";
  // Iterate over window and initialize it. Choose proper base points starting
  // from the end of window.
  uint64_t choosenNum = pow(2, (--checkpoints));
  for (uint64_t i = this->cfgWindowSize; i > 0 ; i--) {
    this->window.push_back(detector::DEFAULT_START_VALUE);

    if (choosenNum == i) {
      checkpointLog << "T-" << choosenNum << " ";

      choosenNum /= 2;
      basePoints.push_back(--this->window.end());
    }
  }
  checkpointLog << "]";

  SERENITY_LOG(INFO) << checkpointLog.str();
}


Result<Detection> SignalDropAnalyzer::processSample(double_t in) {
  // Fill window.
  if (in < 0.1)
    in = 0.1;
  this->window.push_back(in);

  // Process.
  Result<Detection> result = this->_processSample(in);

  // Always at the end of sample process.
  this->shiftBasePoints();
  this->window.pop_front();

  return result;
}


Try<Nothing> SignalDropAnalyzer::resetSignalRecovering() {
  // Return detector to normal state.
  SERENITY_LOG(INFO) << "Resetting any drop tracking if exists.";
  this->valueBeforeDrop = None();

  return Nothing();
}


Result<Detection> SignalDropAnalyzer::_processSample(double_t in) {
  // Check if we track some contention.
  if (this->valueBeforeDrop.isSome()) {
    // Check if the signal returned to normal state. (!)
    double_t nearValue =
      this->cfgNearFraction * this->valueBeforeDrop.get();
    SERENITY_LOG(INFO) << "Waiting for signal: "
                       << in << " to return to: "
                       << (this->valueBeforeDrop.get() - nearValue)
                       << " after corrections. ";
    // We want to use reference Base Point instead of base point.
    if (in >= (this->valueBeforeDrop.get() - nearValue)) {
      SERENITY_LOG(INFO) << "Signal returned to established state.";
      this->resetSignalRecovering();
    } else {
      // Create contention.
      return this->createContention(
        ((this->valueBeforeDrop.get() - nearValue) - in) *
          this->cfgSeverityFraction);
    }
  }

  double_t currentDropFraction = 0;
  double_t meanValueBeforeDrop = 0;
  this->dropVotes = 0;
  std::stringstream basePointValues;

  // Make a voting within all basePoints(checkpoints). Drop will be
  // detected when dropVotes will be >= Quorum number.
  for (std::list<double_t>::iterator basePoint : this->basePoints) {
    basePointValues << " " << (double_t)(*basePoint);

    // Check if drop happened for this basePoint.
    double_t dropFraction = 1.0 - (in / (*basePoint));
    if (dropFraction >= this->cfgFractionalThreshold) {
      // Vote on drop.
      this->dropVotes++;
      currentDropFraction += dropFraction;
      meanValueBeforeDrop += (double_t)(*basePoint);

      basePointValues << "[-] ";
    } else if ((double_t)(*basePoint) >= in) {
      basePointValues << "[~] ";
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
  << " |threshold %: " << this->cfgFractionalThreshold * 100
  << " |dropVotes/quorum: " << this->dropVotes
  << "/" << this->quorumNum
  << "}";

  // Check if drop obtained minimum number of votes.
  if (this->dropVotes >= this->quorumNum) {
    // Create contention.
    this->valueBeforeDrop = meanValueBeforeDrop;
    // TODO(bplotka): Ensure proper severity.
    return this->createContention(
      currentDropFraction * this->cfgSeverityFraction);
  }

  return None();
}

}  // namespace serenity
}  // namespace mesos
