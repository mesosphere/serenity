#include <list>
#include <stdlib.h>

#include <process/dispatch.hpp>
#include <process/process.hpp>

#include <stout/error.hpp>

#include "estimator/serenity_estimator.hpp"

using namespace process;

using std::list;

namespace mesos {
namespace serenity {

class SerenityEstimatorProcess :
    public Process<SerenityEstimatorProcess>
{
public:
  SerenityEstimatorProcess(
      const lambda::function<Future<list<ResourceUsage>>()>& usages_)
  : usages(usages_) {}

  Future<Resources> oversubscribable()
  {
    // TODO(bplotka) Set up the main estimation pipeline here.
    std::cout << "pipe test" << "\n";

    // For now return empty resources.
    return Resources();
  }

private:
  const lambda::function<Future<list<ResourceUsage>>()>& usages;
};


SerenityEstimator::~SerenityEstimator()
{
  if (process.get() != NULL) {
    terminate(process.get());
    wait(process.get());
  }
}


Try<Nothing> SerenityEstimator::initialize(
    const lambda::function<Future<list<ResourceUsage>>()>& usages)
{
  if (process.get() != NULL) {
    return Error("Serenity estimator has already been initialized");
  }

  process.reset(new SerenityEstimatorProcess(usages));
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
