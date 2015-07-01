#ifndef SERENITY_JSON_SOURCE_HPP
#define SERENITY_JSON_SOURCE_HPP

#include <stout/try.hpp>
#include <stout/error.hpp>

#include <mesos/mesos.hpp>

#include <string>

#include "json_source.pb.h"  // NOLINT(build/include)

#include "serenity/serenity.hpp"

namespace mesos {
namespace serenity {
namespace tests {


class JsonSource : public Producer<ResourceUsage> {
 public:
  void RunTests(const std::string& jsonSource);

 protected:
  static const Try<FixtureResourceUsage> ReadJson(
      const std::string& relativePath);
};

}  // namespace tests
}  // namespace serenity
}  // namespace mesos


#endif  // SERENITY_JSON_SOURCE_HPP
