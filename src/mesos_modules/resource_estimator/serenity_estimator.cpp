#include <list>
#include <memory>

#include "glog/logging.h"

#include "mesos_modules/resource_estimator/serenity_estimator.hpp"

#include "pipeline/estimator_pipeline.hpp"

#include "process/defer.hpp"
#include "process/dispatch.hpp"
#include "process/process.hpp"

#include "stout/error.hpp"

// TODO(nnielsen): Break into explicit using-declarations.
using namespace process;  // NOLINT(build/namespaces)

namespace mesos {
namespace serenity {

class SerenityEstimatorProcess :
    public Process<SerenityEstimatorProcess> {
 public:
  SerenityEstimatorProcess(
      const lambda::function<Future<ResourceUsage>()>& _usage,
      std::shared_ptr<ResourceEstimatorPipeline> _pipeline)
    : usage(_usage),
      pipeline(_pipeline) {}

  Future<Resources> oversubscribable() {
    return this->usage()
      .then(defer(self(), &Self::_oversubscribable, lambda::_1));
  }

  Future<Resources> _oversubscribable(
      const Future<ResourceUsage>& _resourceUsage) {
    Resources allocatedRevocable;
    foreach(auto& executor, _resourceUsage.get().executors()) {
      allocatedRevocable += Resources(executor.allocated()).revocable();
    }

    Result<Resources> ret = this->pipeline->run(_resourceUsage.get());

    if (ret.isError()) {
      LOG(ERROR) << ret.error();
      return Resources();
    } else if (ret.isNone()) {
      return Resources();
    }
    LOG(INFO) << "[SerenityEstimator] Considering allocated revocable "
              << "resources: " << allocatedRevocable;
    return (ret.get() - allocatedRevocable);
  }

 private:
  const lambda::function<Future<ResourceUsage>()> usage;
  std::shared_ptr<ResourceEstimatorPipeline> pipeline;
};


SerenityEstimator::~SerenityEstimator() {
  if (process.get() != NULL) {
    terminate(process.get());
    wait(process.get());
  }
}


Try<Nothing> SerenityEstimator::initialize(
    const lambda::function<Future<ResourceUsage>()>& usage) {
  if (process.get() != NULL) {
    return Error("Serenity estimator has already been initialized");
  }

  process.reset(new SerenityEstimatorProcess(usage, this->pipeline));
  spawn(process.get());

  return Nothing();
}


Future<Resources> SerenityEstimator::oversubscribable() {
  if (process.get() == NULL) {
    return Failure("Serenity estimator is not initialized");
  }

  return dispatch(
      process.get(),
      &SerenityEstimatorProcess::oversubscribable);
}

}  // namespace serenity
}  // namespace mesos
