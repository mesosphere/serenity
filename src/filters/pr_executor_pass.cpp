#include <mesos/resources.hpp>

#include "filters/pr_executor_pass.hpp"

namespace mesos {
namespace serenity {

Try<Nothing> PrExecutorPassFilter::consume(const ResourceUsage& in) {
  ResourceUsage prProduct;
  prProduct.mutable_total()->CopyFrom(in.total());
  BeResourceUsage beProduct;
  beProduct.mutable_usage()->mutable_total()->CopyFrom(in.total());
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
      // Add an BE executor.
      ResourceUsage_Executor* outExec =
        beProduct.mutable_usage()->mutable_executors()->Add();
      outExec->CopyFrom(inExec);
    } else {
      // Add an PR executor.
      ResourceUsage_Executor* outExec = prProduct.mutable_executors()->Add();
      outExec->CopyFrom(inExec);
    }
  }

  Producer<ResourceUsage>::produce(prProduct);

  Producer<BeResourceUsage>::produce(beProduct);

  return Nothing();
}

}  // namespace serenity
}  // namespace mesos
