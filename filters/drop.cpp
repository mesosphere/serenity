#include <utility>

#include "filters/drop.hpp"

#include "messages/serenity.hpp"

#include "serenity/wid.hpp"

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
    LOG(INFO) << "[SerenityQoS] DropDetector: Found decrease, "
                 "but not significant: " << (in - basePoint);
  }

  return None();
}


template <typename T>
Try<Nothing> DropFilter<T>::consume(const ResourceUsage& in) {
  std::unique_ptr<ExecutorSet> newSamples(new ExecutorSet());
  Contentions product;

  for (ResourceUsage_Executor inExec : in.executors()) {
    if (!inExec.has_executor_info()) {
      SERENITY_LOG(ERROR) << "Executor <unknown>"
                 << " does not include executor_info";
      // Filter out these executors.
      continue;
    }
    if (!inExec.has_statistics()) {
      SERENITY_LOG(ERROR) << "Executor "
                 << inExec.executor_info().executor_id().value()
                 << " does not include statistics.";
      // Filter out these executors.
      continue;
    }
    newSamples->insert(inExec);

    // Check if change point Detector for given executor exists.
    auto cpDetector = this->cpDetectors->find(inExec.executor_info());
    if (cpDetector == this->cpDetectors->end()) {
      // If not insert new one.
      auto pair = std::pair<ExecutorInfo, T*>(inExec.executor_info(), new T());
      pair.second->configure(this->changePointDetectionState);
      this->cpDetectors->insert(pair);

    } else {
      // Check if previousSample for given executor exists.
      auto previousSample = this->previousSamples->find(inExec);
      if (previousSample != this->previousSamples->end()) {
        // Get proper value.
        Try<double_t> value = this->valueGetFunction((*previousSample), inExec);
        if (value.isError()) {
          SERENITY_LOG(ERROR)  << value.error();
          continue;
        }

        // Perform change point detection.
        Result<ChangePointDetection> cpDetected =
            (cpDetector->second)->processSample(value.get());
        if (cpDetected.isError()) {
          SERENITY_LOG(ERROR)  << cpDetected.error();
          continue;
        }

        // Detected change point.
        if (cpDetected.isSome()) {
          product.push_back(createCpuContention(
              cpDetected.get().severity,
              WID(inExec.executor_info()).getWorkID(),
              inExec.statistics().timestamp()));
        }
      }
    }
  }

  this->previousSamples->clear();
  this->previousSamples = std::move(newSamples);

  // Continue pipeline.
  produce(product);

  return Nothing();
}


// Fix for using templated methods in .cpp file.
// See:
//    https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template class DropFilter<NaiveChangePointDetector>;
template class DropFilter<RollingChangePointDetector>;

}  // namespace serenity
}  // namespace mesos
