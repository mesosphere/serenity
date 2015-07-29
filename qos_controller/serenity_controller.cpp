#include <list>
#include <memory>

#include "glog/logging.h"

#include "messages/serenity.hpp"

#include "pipeline/qos_pipeline.hpp"

#include "process/defer.hpp"
#include "process/delay.hpp"
#include "process/dispatch.hpp"
#include "process/process.hpp"

#include "stout/error.hpp"

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
      const lambda::function<Future<ResourceUsage>()>& _usage,
      std::shared_ptr<QoSControllerPipeline> _pipeline)
    : usage(_usage), pipeline(_pipeline) {}

  Future<QoSCorrections> corrections() {
    return this->usage()
      .then(defer(self(), &Self::_corrections, lambda::_1));
  }

  Future<QoSCorrections> _corrections(
      const Future<ResourceUsage>& _resourceUsage) {
    QoSCorrections corrections = this->__corrections(_resourceUsage);

    // TODO(bplotka): We don't want to spam slave with empty corrections.
    // We could make 20 iterations and try to find correction like this:
    //    this->iterations = 0;
    //    while(corrections.empty() && this->iterations < 20) {
    //      std::cout << "xxxx "  << std::endl;
    //
    //      os::sleep(Duration::create(60).get());
    //
    //      ResourceUsage usage = this->usage().get();
    //      corrections = this->__corrections(usage);
    //      this->iterations++;
    //    }
    // I would like to hear your opinions about that first.

    return corrections;
  }

  QoSCorrections __corrections(
      const Future<ResourceUsage>& _resourceUsage) {
    Try<QoSCorrections> ret = this->pipeline->run(_resourceUsage.get());

    QoSCorrections corrections;

    if (ret.isError()) {
      LOG(ERROR) << ret.error();
      // Return empty corrections.
    } else {
      corrections = ret.get();
    }

    return corrections;
  }

 private:
  const lambda::function<Future<ResourceUsage>()> usage;
  std::shared_ptr<QoSControllerPipeline> pipeline;
  //! Safeguard against infinite loop.
  uint64_t iterations = 0;
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

  process.reset(new SerenityControllerProcess(usage, this->pipeline));
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
