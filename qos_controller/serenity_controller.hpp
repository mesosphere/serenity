#ifndef SERENITY_SERENITY_QOS_CONTROLLER_HPP
#define SERENITY_SERENITY_QOS_CONTROLLER_HPP

#include <list>

#include <mesos/slave/oversubscription.pb.h> // ONLY USEFUL AFTER RUNNING PROTOC
#include <mesos/slave/qos_controller.hpp>

#include <stout/lambda.hpp>
#include <stout/nothing.hpp>
#include <stout/try.hpp>

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {

// Forward declaration.
class SerenityControllerProcess;


class SerenityController: public slave::QoSController
{
public:
  SerenityController() {};

  static Try<slave::QoSController*>create(const Option<std::string>& type)
  {
    return new SerenityController();
  }

  virtual ~SerenityController();

  virtual Try<Nothing> initialize(
      const lambda::function<process::Future<ResourceUsage>()>& usage);

  virtual process::Future<std::list<slave::QoSCorrection>> corrections();

protected:
  process::Owned<SerenityControllerProcess> process;
};

} // namespace serenity
} // namespace mesos

#endif //SERENITY_SERENITY_QOS_CONTROLLER_HPP
