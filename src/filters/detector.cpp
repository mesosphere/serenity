#include <utility>

#include "filters/detector.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> DetectorFilter::consume(const ResourceUsage& usage) {
  this->currentVictimsUsage = usage;

  if (this->currentAggressorsUsage.isSome()) {
    this->__detect();
  }

  return Nothing();
}


Try<Nothing> DetectorFilter::consume(const BeResourceUsage& usage) {
  this->currentAggressorsUsage = usage.usage();

  if (this->currentVictimsUsage.isSome()) {
    this->__detect();
  }

  return Nothing();
}

/**
 *
 */
Try<Nothing> DetectorFilter::__detect() {
  std::unique_ptr<ExecutorSet> newSamples(new ExecutorSet());
  const ResourceUsage currentAggressors = this->currentAggressorsUsage.get();
  const ResourceUsage currentVictims = this->currentVictimsUsage.get();
  Contentions product;

  for (ResourceUsage_Executor inExec : currentVictims.executors()) {
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
    auto cpDetector = this->detectors->find(inExec.executor_info());
    if (cpDetector == this->detectors->end()) {
      // If not insert new detector using detector factory..
      auto pair = std::pair<ExecutorInfo, std::shared_ptr<BaseDetector>>(
        inExec.executor_info(),
        std::make_shared<BaseDetector>(AssuranceDetector(tag,
                                                         this->detectorConf)));

      this->detectors->insert(pair);

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
        Result<Detection> cpDetected =
            (cpDetector->second)->processSample(value.get());
        if (cpDetected.isError()) {
          SERENITY_LOG(ERROR)  << cpDetected.error();
          continue;
        }

        // Detected contention.
        if (cpDetected.isSome()) {
          // Check if aggressors jobs are available on host.
          if (currentAggressors.executors_size() == 0) {
            SERENITY_LOG(INFO) << "Contention spotted, however there are no "
                << "aggressor on the host. Assuming false positive.";
            (cpDetector->second)->reset();
          }

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

  this->currentVictimsUsage = None();
  this->currentAggressorsUsage = None();

  return Nothing();
}


}  // namespace serenity
}  // namespace mesos
