#include <utility>

#include "filters/drop.hpp"

#include "messages/serenity.hpp"

#include "serenity/wid.hpp"

#include "stout/none.hpp"

namespace mesos {
namespace serenity {

Result<ChangePointDetection> NaiveChangePointDetector::processSample(
    double_t in) {

  if (in < this->absoluteThreshold) {
    ChangePointDetection cpd;
    cpd.severity = this->absoluteThreshold - in;

    return cpd;
  }

  return None();
}


template <class T>
Try<Nothing> DropFilter<T>::consume(const ResourceUsage& in) {
  std::unique_ptr<ExecutorSet> newSamples(new ExecutorSet());
  Contentions product;

  for (ResourceUsage_Executor inExec : in.executors()) {
    if (!inExec.has_executor_info()) {
      LOG(ERROR) << "Executor <unknown>"
      << " does not include executor_info";
      // Filter out these executors.
      continue;
    }
    if (!inExec.has_statistics()) {
      LOG(ERROR) << "Executor "
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
      cpDetectors->insert(std::pair<ExecutorInfo, T*>(
          inExec.executor_info(), new T(this->windowSize,
                                        this->absoluteThreshold)));

    } else {
      // Check if previousSample for given executor exists.
      auto previousSample = this->previousSamples->find(inExec);
      if (previousSample != this->previousSamples->end()) {
        // Get proper value.
        Try<double_t> value = this->valueGetFunction((*previousSample), inExec);
        if (value.isError()) {
          LOG(ERROR) << value.error();
          continue;
        }

        // Perform change point detection.
        Result<ChangePointDetection> cpDetected =
            (cpDetector->second)->processSample(value.get());
        if (cpDetected.isError()) {
          LOG(ERROR) << cpDetected.error();
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

  produce(product);

  return Nothing();
}


// Fix for using templated methods in .cpp file.
// See:
//    https://isocpp.org/wiki/faq/templates#separate-template-fn-defn-from-decl
template class DropFilter<NaiveChangePointDetector>;

}  // namespace serenity
}  // namespace mesos
