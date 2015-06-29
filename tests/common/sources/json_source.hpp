#ifndef SERENITY_JSON_SOURCE_HPP
#define SERENITY_JSON_SOURCE_HPP

#include <mesos/mesos.hpp>

#include <stout/error.hpp>
#include <stout/lambda.hpp>
#include <stout/try.hpp>

#include <string>

#include "json_source.pb.h"  // NOLINT(build/include)

#include "serenity/serenity.hpp"



namespace mesos {
namespace serenity {
namespace tests {


class JsonSource : public Producer<ResourceUsage> {
 public:
  JsonSource() {}

  explicit JsonSource(Consumer<ResourceUsage>* _consumer) {
    addConsumer(_consumer);
  }

  Try<Nothing> RunTests(const std::string& jsonSource);

 protected:
  static const Try<FixtureResourceUsage> ReadJson(
      const std::string& relativePath);
};

}  // namespace tests
}  // namespace serenity
}  // namespace mesos


#endif  // SERENITY_JSON_SOURCE_HPP
