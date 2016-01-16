#ifndef SERENITY_PIPELINE_HPP
#define SERENITY_PIPELINE_HPP

#include "bus/event_bus.hpp"

#include "serenity/serenity.hpp"

#include "stout/error.hpp"
#include "stout/nothing.hpp"
#include "stout/result.hpp"
#include "stout/try.hpp"

namespace mesos {
namespace serenity {

/**
 * Base class for pipeline. It becomes source and sink in the
 * same time to integrate with filters within the module.
 * In order to introduce a new pipeline using this base class, filters need
 * to be connected to this instance at the beginning and the end of pipeline.
 *
 * Product is a type of object for pipeline feed.
 * Consumable is a type of object which is consumed at the end of pipeline.
 */
template<typename Product, typename Consumable>
class Pipeline : public Producer<Product>, public Consumer<Consumable> {
 public:
  virtual ~Pipeline() {}

  virtual Result<Consumable> run(const Product& _product) {
    // Reset result.
    this->result = None();

    // Start pipeline.
    Try<Nothing> ret = this->produce(_product);

    Try<Nothing> retPostRun = this->postPipelineRun();

    if (ret.isError()) {
      return Error(ret.error());
    }

    if (retPostRun.isError()) {
      return Error(retPostRun.error());
    }

    return this->result;
  }

  virtual Try<Nothing> consume(const Consumable& in) {
    // Save consumed product at the end of pipeline as a result.
    this->result = in;

    return Nothing();
  }

  //! Place for additional logic after pipeline run.
  // Usually we need to ensure that in case of error or empty result we reset
  // sync consumers to be ready for next iteration. Some of them are crucial
  // and need to forced to continue the pipeline.
  // Override if you have sync consumers in you pipeline.
  // TODO(bplotka): That would not be needed if we continue pipeline always.
  virtual Try<Nothing> postPipelineRun() {
    return Nothing();
  }

 protected:
  Option<Consumable> result;
};

using QoSControllerPipeline = Pipeline<ResourceUsage, QoSCorrections>;

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_PIPELINE_HPP
