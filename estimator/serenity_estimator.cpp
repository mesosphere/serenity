#include <process/defer.hpp>
#include <process/dispatch.hpp>
#include <process/process.hpp>

#include <stout/error.hpp>

#include <list>

#include "estimator/serenity_estimator.hpp"

// TODO(nnielsen): Break into explicit using-declarations.
using namespace process;  // NOLINT(build/namespaces)

namespace mesos {
namespace serenity {

class SerenityEstimatorProcess :
    public Process<SerenityEstimatorProcess> {
 public:
  SerenityEstimatorProcess(
      const lambda::function<Future<ResourceUsage>()>& _usage)
  : usage(_usage) {}

  Future<Resources> oversubscribable() {
    return this->usage()
      .then(defer(self(), &Self::_oversubscribable, lambda::_1));
  }

  Future<Resources> _oversubscribable(
      const Future<ResourceUsage>& _resourceUsage) {
    // TODO(bplotka) Set up the main estimation pipeline here.
    std::cout << "Serenity Estimator pipeline run." << "\n";

    // For now return empty Resources.
    return Resources();
  }

 private:
  const lambda::function<Future<ResourceUsage>()> usage;
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

  process.reset(new SerenityEstimatorProcess(usage));
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
