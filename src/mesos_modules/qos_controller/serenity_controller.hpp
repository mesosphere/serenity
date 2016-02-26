#ifndef SERENITY_SERENITY_QOS_CONTROLLER_HPP
#define SERENITY_SERENITY_QOS_CONTROLLER_HPP

#include <list>
#include <memory>
#include <string>

#include "mesos/slave/oversubscription.pb.h"  // ONLY USEFUL AFTER RUNNING ROTOC
#include "mesos/slave/qos_controller.hpp"

#include "pipeline/qos_pipeline.hpp"

#include "serenity/serenity.hpp"

#include "stout/lambda.hpp"
#include "stout/nothing.hpp"
#include "stout/try.hpp"

namespace mesos {
namespace serenity {

// Forward declaration.
class SerenityControllerProcess;


class SerenityController: public slave::QoSController {
 public:
  explicit SerenityController(
      std::unique_ptr<QoSControllerPipeline> _pipeline,
      double _onEmptyCorrectionInterval)
    : pipeline(std::move(_pipeline)),
      onEmptyCorrectionInterval(_onEmptyCorrectionInterval) {}

  static Try<slave::QoSController*> create(
      std::unique_ptr<QoSControllerPipeline> _pipeline,
      double _onEmptyCorrectionInterval = 5) {
    return new SerenityController(std::move(_pipeline),
                                  _onEmptyCorrectionInterval);
  }

  virtual ~SerenityController();

  virtual Try<Nothing> initialize(
      const lambda::function<process::Future<ResourceUsage>()>& usage);

  virtual process::Future<std::list<slave::QoSCorrection>> corrections();

 protected:
  process::Owned<SerenityControllerProcess> process;
  std::unique_ptr<QoSControllerPipeline> pipeline;
  double onEmptyCorrectionInterval;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_SERENITY_QOS_CONTROLLER_HPP
