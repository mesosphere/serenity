#include <list>
#include <stdlib.h>

#include <process/dispatch.hpp>
#include <process/process.hpp>

#include <stout/error.hpp>

#include "qos_controller/serenity_controller.hpp"

using namespace process;

using std::list;

using mesos::slave::QoSCorrection;

namespace mesos {
namespace serenity {

class SerenityControllerProcess :
    public Process<SerenityControllerProcess>
{
public:
  SerenityControllerProcess(
      const lambda::function<Future<ResourceUsage>()>& usage_)
      : usage(usage_) {}

  Future<list<QoSCorrection>> corrections()
  {
    // TODO(bplotka) Set up the main qos correction pipeline here.
    std::cout << "pipe test" << "\n";

    // For now return empty resources.
    return list<QoSCorrection>();
  }

private:
  const lambda::function<Future<ResourceUsage>()>& usage;
};


SerenityController::~SerenityController()
{
  if (process.get() != NULL) {
    terminate(process.get());
    wait(process.get());
  }
}


Try<Nothing> SerenityController::initialize(
    const lambda::function<Future<ResourceUsage>()>& usage)
{
  if (process.get() != NULL) {
    return Error("Serenity QoS Controller has already been initialized");
  }

  process.reset(new SerenityControllerProcess(usage));
  spawn(process.get());

  return Nothing();
}


Future<list<QoSCorrection>> SerenityController::corrections()
{
  if (process.get() == NULL) {
    return Failure("Serenity QoS Correction is not initialized");
  }

  return dispatch(
      process.get(),
      &SerenityControllerProcess::corrections);
}

} // namespace serenity {
} // namespace mesos {
