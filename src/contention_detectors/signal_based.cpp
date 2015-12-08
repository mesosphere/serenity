#include <utility>

#include "contention_detectors/signal_based.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> SignalBasedDetector::consume(const ResourceUsage& in) {
  return this->_detect(DividedResourceUsage(in));
}


Try<Nothing> SignalBasedDetector::_detect(
    const DividedResourceUsage& usage) {
  Contentions product;

  for (const ResourceUsage_Executor& inExec : usage.prExecutors()) {
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

    // Check if change point Detector for given executor exists.
    auto cpDetector = this->detectors->find(inExec.executor_info());
    if (cpDetector == this->detectors->end()) {
      // If not insert new detector using detector factory..
      // TODO(bplotka): Construct such factory. For now take Assurance.
      this->detectors->insert(std::pair<ExecutorInfo,
        std::unique_ptr<SignalAnalyzer>>(
        inExec.executor_info(),
        std::unique_ptr<SignalAnalyzer>(new SignalDropAnalyzer(
          tag, this->detectorConf))));

    } else {
      // Check if previousSample for given executor exists.
      // Get proper value.
      Try<double_t> value = this->valueGetFunction(inExec);
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
        if (usage.beExecutors().size() == 0) {
          SERENITY_LOG(INFO) << "Contention spotted, however there are no "
              << "Best effort tasks on the host. Assuming false positive.";
          (cpDetector->second)->reset();
        }

        product.push_back(createCpuContention(
            cpDetected.get().severity,
            WID(inExec.executor_info()).getWorkID(),
            inExec.statistics().timestamp()));
      }
    }
  }

  // Continue pipeline.
  produce(product);

  return Nothing();
}


}  // namespace serenity
}  // namespace mesos
