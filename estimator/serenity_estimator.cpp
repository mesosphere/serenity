#include <list>
#include <stdlib.h>

#include <process/dispatch.hpp>
#include <process/process.hpp>

#include <stout/error.hpp>

#include "estimator/serenity_estimator.hpp"

using namespace process;

namespace mesos {
namespace serenity {

class SerenityEstimatorProcess :
    public Process<SerenityEstimatorProcess>
{
public:
  SerenityEstimatorProcess(
      const lambda::function<Future<ResourceUsage>()>& _usage)
  : usage(_usage) {}

  Future<Resources> oversubscribable()
  {
    // TODO(bplotka) Set up the main estimation pipeline here.
    std::cout << "Serenity Estimator pipeline run." << "\n";

    // For now return empty resources.
    return Resources();
  }

private:
  const lambda::function<Future<ResourceUsage>()>& usage;
};


SerenityEstimator::~SerenityEstimator()
{
  if (process.get() != NULL) {
    terminate(process.get());
    wait(process.get());
  }
}


Try<Nothing> SerenityEstimator::initialize(
    const lambda::function<Future<ResourceUsage>()>& usage)
{
  if (process.get() != NULL) {
    return Error("Serenity estimator has already been initialized");
  }

  process.reset(new SerenityEstimatorProcess(usage));
  spawn(process.get());

  return Nothing();
}


Future<Resources> SerenityEstimator::oversubscribable()
{
  if (process.get() == NULL) {
    return Failure("Serenity estimator is not initialized");
  }

  return dispatch(
      process.get(),
      &SerenityEstimatorProcess::oversubscribable);
}

} // namespace serenity {
} // namespace mesos {
