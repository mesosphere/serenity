#include <stdlib.h>

#include <process/dispatch.hpp>
#include <process/process.hpp>

#include <stout/error.hpp>

#include <list>

#include "qos_controller/serenity_controller.hpp"

// TODO(nnielsen): Break up with explicit using-declarations instead.
using namespace process;  // NOLINT(build/namespaces)

using std::list;

using mesos::slave::QoSCorrection;

namespace mesos {
namespace serenity {

class SerenityControllerProcess :
    public Process<SerenityControllerProcess> {
 public:
  SerenityControllerProcess(
      const lambda::function<Future<ResourceUsage>()>& _usage)
      : usage(_usage) {}

  Future<list<QoSCorrection>> corrections() {
    // TODO(bplotka) Set up the main qos correction pipeline here.
    std::cout << "Serenity QoS Controller pipeline run." << "\n";
    // For now return empty resources.
    return Future<list<QoSCorrection>>();
  }

 private:
  const lambda::function<Future<ResourceUsage>()>& usage;
};


SerenityController::~SerenityController() {
  if (process.get() != NULL) {
    terminate(process.get());
    wait(process.get());
  }
}


Try<Nothing> SerenityController::initialize(
    const lambda::function<Future<ResourceUsage>()>& usage) {
  if (process.get() != NULL) {
    return Error("Serenity QoS Controller has already been initialized");
  }

  process.reset(new SerenityControllerProcess(usage));
  spawn(process.get());

  return Nothing();
}


Future<list<QoSCorrection>> SerenityController::corrections() {
  if (process.get() == NULL) {
    return Failure("Serenity QoS Correction is not initialized");
  }

  return dispatch(
      process.get(),
      &SerenityControllerProcess::corrections);
}

}  // namespace serenity
}  // namespace mesos
