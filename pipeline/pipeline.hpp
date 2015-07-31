#ifndef SERENITY_PIPELINE_HPP
#define SERENITY_PIPELINE_HPP

#include "serenity/serenity.hpp"

#include "stout/error.hpp"
#include "stout/try.hpp"
#include "stout/nothing.hpp"

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

  virtual Try<Consumable> run(const Product& _product) {
    // Reset result.
    this->result = None();

    // Start pipeline.
    Try<Nothing> ret = this->produce(_product);
    if (ret.isError()) {
      return Error(ret.error());
    }

    // Proper pipeline should fill result at the end.
    // (run this->consume(..))
    if (this->result.isNone()) {
      // End of pipeline did not consume anything.
      return Error(
          "[Serenity] Pipeline is blocked - haven't got any "
              "consumable from pipeline.");
    }

    return this->result.get();
  }

  virtual Try<Nothing> consume(const Consumable& in) {
    // Save consumed product at the end of pipeline as a result.
    this->result = in;

    return Nothing();
  }

 protected:
  Option<Consumable> result;
};

}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_PIPELINE_HPP
