#ifndef SERENITY_PUBLIC_SOURCE_HPP
#define SERENITY_PUBLIC_SOURCE_HPP

#include <mesos/mesos.hpp>

#include <stout/try.hpp>

#include "json_source.pb.h"  // NOLINT(build/include)

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {
namespace tests {


class PublicSource : public Producer<ResourceUsage> {
 public:
  PublicSource() {}

  explicit PublicSource(Consumer<ResourceUsage>* _consumer) {
    addConsumer(_consumer);
  }

  Try<Nothing> produceUsage(ResourceUsage out) {
    produce(out);

    return Nothing();
  }
};

}  // namespace tests
}  // namespace serenity
}  // namespace mesos


#endif  // SERENITY_PUBLIC_SOURCE_HPP
