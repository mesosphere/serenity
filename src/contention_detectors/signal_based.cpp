#include <list>
#include <utility>

#include "contention_detectors/signal_based.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> SignalBasedDetector::consume(const ResourceUsage& usage) {
  auto executorsListsTuple =
    ResourceUsageHelper::getProductionAndRevocableExecutors(usage);

  std::list<ResourceUsage_Executor> productionExecutors =
    std::get<ResourceUsageHelper::ExecutorType::PRODUCTION>(
      executorsListsTuple);
  std::list<ResourceUsage_Executor> revocableExecutors =
    std::get<ResourceUsageHelper::ExecutorType::REVOCABLE>(
      executorsListsTuple);

  SERENITY_LOG(INFO) << "Production executors: " << productionExecutors.size()
                    << " | Revocable executors: " << revocableExecutors.size();

  Contentions product;
  for (const ResourceUsage_Executor& executor : productionExecutors) {
    if (!ResourceUsageHelper::isExecutorHasStatistics(executor)) {
      SERENITY_LOG(INFO) << "No statistics for executor " <<
      executor.executor_info().command();
      continue;
    }

    // Check if change point Detector for given executor exists.
    auto cpDetector = this->detectors.find(executor.executor_info());
    if (cpDetector == this->detectors.end()) {
      this->detectors.insert(
      std::pair<ExecutorInfo, SignalAnalyzer>(
      executor.executor_info(),
      SignalDropAnalyzer(tag, this->detectorConf)));
    } else {
      // Check if previousSample for given executor exists.
      // Get proper value.
      Try<double_t> value = this->getValue(executor);
      if (value.isError()) {
        SERENITY_LOG(ERROR) << value.error();
        continue;
      }

      // Perform change point detection.
      Result<Detection> cpDetected =
      (cpDetector->second).processSample(value.get());
      if (cpDetected.isError()) {
        SERENITY_LOG(ERROR) << cpDetected.error();
        continue;
      }

      // Detected contention.
      if (cpDetected.isSome()) {
        if (revocableExecutors.empty()) {
          SERENITY_LOG(INFO) << "Contention spotted, however there are no "
                  << "Best effort tasks on the host. Assuming false positive";
          (cpDetector->second).reset();
        } else {
          SERENITY_LOG(INFO) << "Signal contention spotted";
          product.push_back(createContention(
          cpDetected.get().severity,
          contentionType,
          WID(executor.executor_info()).getWorkID(),
          executor.statistics().timestamp()));
        }
      }
    }
  }
  SERENITY_LOG(INFO) << "Producing " << product.size() << " contentions";
  produce(product);
  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
