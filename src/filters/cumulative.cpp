#include <atomic>
#include <string>
#include <utility>

#include "filters/cumulative.hpp"

#include "glog/logging.h"

#include "mesos/mesos.hpp"

namespace mesos {
namespace serenity {

using std::map;
using std::pair;
using std::string;


CumulativeFilter::~CumulativeFilter() {}


Try<Nothing> CumulativeFilter::consume(const ResourceUsage& in) {
  std::unique_ptr<ExecutorSet> newSamples(new ExecutorSet());
  double_t totalCpuUsage = 0;
  ResourceUsage product;

  for (const ResourceUsage_Executor& inExec : in.executors()) {
    string executor_id = "<unknown>";

    if (!inExec.has_executor_info()) {
      SERENITY_LOG(ERROR) << "Executor " << executor_id
      << " does not include executor_info. ";
      // Filter out these executors.
      continue;
    } else {
      executor_id = inExec.executor_info().executor_id().value();
    }

    if (!inExec.has_statistics()) {
      SERENITY_LOG(ERROR) << "Executor " << executor_id
      << " does not include statistics. ";
      // Filter out these executors.
      continue;
    }

    if (inExec.has_executor_info() && inExec.has_statistics()) {
      newSamples->insert(inExec);

      auto previousSample = this->previousSamples->find(inExec);
      if (previousSample != this->previousSamples->end()) {
        // Cumulate to sample conversion.
        ResourceUsage_Executor* outExec = new ResourceUsage_Executor(inExec);

        // Convert timestamp.
        // NOTE(bplotka): Make sure we don't use timestamp as absolute counter
        // in next filters.
        if (previousSample->statistics().has_timestamp() &&
            inExec.statistics().has_timestamp()) {
          double_t sampled =
            previousSample->statistics().timestamp() -
              inExec.statistics().timestamp();
          outExec->mutable_statistics()->set_timestamp(sampled);
        }

        // Convert cpus_system_time_secs.
        if (previousSample->statistics().has_cpus_system_time_secs() &&
            inExec.statistics().has_cpus_system_time_secs()) {
          double_t sampled =
            previousSample->statistics().cpus_system_time_secs() -
            inExec.statistics().cpus_system_time_secs();
          outExec->mutable_statistics()->set_cpus_system_time_secs(sampled);
        }

        // Convert cpus_user_time_secs.
        if (previousSample->statistics().has_cpus_user_time_secs() &&
            inExec.statistics().has_cpus_user_time_secs()) {
          double_t sampled =
            previousSample->statistics().cpus_user_time_secs() -
            inExec.statistics().cpus_user_time_secs();
          outExec->mutable_statistics()->set_cpus_user_time_secs(sampled);
        }

        product.mutable_executors()->AddAllocated(outExec);

      } else {
        // TODO(bplotka): Does it make sense to assume 0 as previous value?
        // (Are these values counted from 0)?
        // If yes we can continue pipeline with these:
        SERENITY_LOG(INFO) << "First iteration for Executor " << executor_id;
        ResourceUsage_Executor* outExec = new ResourceUsage_Executor(inExec);
        product.mutable_executors()->AddAllocated(outExec);
      }
    }
  }

  this->previousSamples->clear();
  this->previousSamples = std::move(newSamples);

  if (this->previousSamples->empty()) {
    SERENITY_LOG(INFO)
    << "There is no Executor in given usage. Ending the pipeline.";
    return Nothing();
  }

  // Continue pipeline.
  SERENITY_LOG(INFO) << "Continuing with "
  << product.executors_size() << " executor(s).";
  produce(product);

  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
