#include "filters/drop.hpp"

namespace mesos {
namespace serenity {

virtual Try<bool> MeanChangePointDetector::processSample(
    const double_t& in) {

  return false;
}

DropFilter::~DropFilter() {}


template <class T>
Try<Nothing> DropFilter::consume(const ResourceUsage& in) {
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
      T detector();
      cpDetectors->insert(std::pair<ExecutorInfo, T>(
          inExec.executor_info(), detector));

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
        Try<bool> cpDetected =
            (cpDetector->second).processSample(value.get());
        if (cpDetected.isError()) {
          LOG(ERROR) << cpDetected.error();
          continue;
        }

        // TODO(bplotka) Create contention if needed
        // to product.
      }
    }
  }

  this->previousSamples->clear();
  this->previousSamples = std::move(newSamples);

  produce(product);

  return Nothing();
}


}  // namespace serenity
}  // namespace mesos
