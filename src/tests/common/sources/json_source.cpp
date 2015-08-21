#include <mesos/mesos.hpp>

#include <pbjson.hpp>

#include <stout/os.hpp>

#include <string>

#include "tests/common/sources/json_source.hpp"

#include "json_source.pb.h"  // NOLINT(build/include)

namespace mesos {
namespace serenity {
namespace tests {

Try<Nothing> JsonSource::RunTests(const std::string& jsonSource) {
  Try<mesos::FixtureResourceUsage> usages = JsonSource::ReadJson(jsonSource);
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  for (auto itr = usages.get().resource_usage().begin();
      itr != usages.get().resource_usage().end();
      itr++) {
    Try<Nothing> ret = produce(*itr);

    // Stop the pipeline in case of error.
    if (ret.isError()) return ret;
  }

  return Nothing();
}

const Try<FixtureResourceUsage> JsonSource::ReadJson(
    const std::string& relativePath) {
  Try<std::string> content = os::read(relativePath);
  if (content.isError()) {
    return Error("Read error: " + content.error());
  } else if (!content.isSome()) {
    return Error("Readed file is none");
  }

  std::string err;
  FixtureResourceUsage usages;
  int reply = pbjson::json2pb(content.get(), &usages, err);
  if (reply != 0) {
    Try<std::string> emsg = strings::format(
        "Error during json deserialization| errno: %d | err: %s", reply, err);
    return Error(emsg.get());
  }

  return usages;
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

