#include <list>
#include <memory>

#include "glog/logging.h"

#include "mesos_modules/qos_controller/serenity_controller.hpp"

#include "messages/serenity.hpp"

#include "pipeline/qos_pipeline.hpp"

#include "process/defer.hpp"
#include "process/delay.hpp"
#include "process/dispatch.hpp"
#include "process/process.hpp"

#include "stout/error.hpp"
#include "stout/os.hpp"

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
      std::shared_ptr<QoSControllerPipeline> _pipeline,
      double _onEmptyCorrectionInterval)
    : usage(_usage),
      pipeline(_pipeline),
      onEmptyCorrectionInterval(_onEmptyCorrectionInterval) {}

  Future<QoSCorrections> corrections() {
    return this->usage()
      .then(defer(self(), &Self::_corrections, lambda::_1));
  }

  Future<QoSCorrections> _corrections(
      const Future<ResourceUsage>& _resourceUsage) {
    QoSCorrections corrections = this->__corrections(_resourceUsage);

    // TODO(bplotka): We don't want to spam slave with empty corrections.
    // We could set qos_correction_interval_min to longer duration than
    // 0ns, however that would introduce some latency for our QoS controller.
    // We make 20 iterations and try to find correction:
    this->iterations = 0;
    // TODO(bplotka): Filter out the same corrections as in previous message
    while (corrections.empty() && this->iterations < 20) {
      // TODO(bplotka): For tests we need ability to mock sleep.
      os::sleep(Duration::create(onEmptyCorrectionInterval).get());

      ResourceUsage usage = this->usage().get();
      corrections = this->__corrections(usage);
      this->iterations++;
    }
    return corrections;
  }

  QoSCorrections __corrections(
      const Future<ResourceUsage>& _resourceUsage) {
    LOG(INFO) << "[SerenityQoS] -------- Starting QoS pipeline --------";
    Result<QoSCorrections> ret = this->pipeline->run(_resourceUsage.get());

    QoSCorrections corrections;
    if (ret.isError()) {
      LOG(ERROR) << ret.error();
      // Return empty corrections.
    } else if (ret.isSome()) {
      corrections = ret.get();
    }

    LOG(INFO) << "[SerenityQoS] ------- Ending QoS pipeline with "
              << corrections.size() << " corrections. --------";
    return corrections;
  }

 private:
  const lambda::function<Future<ResourceUsage>()> usage;
  std::shared_ptr<QoSControllerPipeline> pipeline;
  //! How much time we wait in case of empty correction.
  //! This value should be near the perf interval since it is useless
  //! to rerun QoS pipeline on the same perf's counter collection.
  double onEmptyCorrectionInterval;
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

  process.reset(new SerenityControllerProcess(
      usage, this->pipeline, this->onEmptyCorrectionInterval));
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
