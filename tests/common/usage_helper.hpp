#ifndef SERENITY_TESTS_USAGE_HELPER_HPP
#define SERENITY_TESTS_USAGE_HELPER_HPP

#include <gtest/gtest.h>

#include <mesos/mesos.pb.h>
#include <mesos/resources.hpp>

#include <process/future.hpp>

#include <pbjson.hpp>

#include <stout/os.hpp>

#include <string>

#include "json_source.pb.h"  // NOLINT(build/include)

namespace mesos {
namespace serenity {
namespace tests {

const std::string SERENITY_FIXTURES_DIR = "tests/fixtures/";

class JsonUsage {
 public:
  static const Try<FixtureResourceUsage> ReadJson(
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
          "Error during json deserialization | errno: %d | err: %s",
          reply,
          err);
      return Error(emsg.get());
    }

    return usages;
  }
};


/**
 * Fake usage function (same method as in mesos::slave::Slave).
 * For internal unit tests use JsonSource.
 */
class MockSlaveUsage {
 public:
  explicit MockSlaveUsage(const std::string& jsonSource) {
    Try<mesos::FixtureResourceUsage> usages = JsonUsage::ReadJson(jsonSource);
    if (usages.isError()) {
      LOG(ERROR) << "Json Usage failed: " << usages.error() << std::endl;
    }

    results = usages.get();
  }

  process::Future<ResourceUsage> usage() {
    if (iteration >= results.resource_usage_size())
      return ResourceUsage();
    std::cout<< iteration << std::endl;
    return results.resource_usage(iteration++);
  }

  process::Future<ResourceUsage> usageIter(int _iteration) {
    if (_iteration >= results.resource_usage_size())
      return ResourceUsage();
    std::cout<< _iteration << std::endl;
    return results.resource_usage(_iteration);
  }

 private:
  mesos::FixtureResourceUsage results;
  int iteration = 0;
};

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

#endif  // SERENITY_TESTS_USAGE_HELPER_HPP
