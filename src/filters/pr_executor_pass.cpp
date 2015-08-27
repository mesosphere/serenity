#include <mesos/resources.hpp>

#include "filters/pr_executor_pass.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> PrExecutorPassFilter::consume(const ResourceUsage& in) {
  ResourceUsage product;
  for (ResourceUsage_Executor inExec : in.executors()) {
    if (!inExec.has_executor_info()) {
      LOG(ERROR) << name << "Executor <unknown>"
                 << " does not include executor_info";
      // Filter out these executors.
      continue;
    }
    if (inExec.allocated().size() == 0) {
      LOG(ERROR) << name << "Executor "
      << inExec.executor_info().executor_id().value()
      << " does not include allocated resources.";
      // Filter out these executors.
      continue;
    }

    Resources allocated(inExec.allocated());
    // Check if task uses revocable resources.
    if (!allocated.revocable().empty()) {
      // Filter out BE tasks.
      continue;
    }

    // Add an executor only when there was no error.
    ResourceUsage_Executor* outExec = product.mutable_executors()->Add();
    outExec->CopyFrom(inExec);
  }

  if (0 != product.executors_size()) {
    // Continue pipeline.
    produce(product);
  }


  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
