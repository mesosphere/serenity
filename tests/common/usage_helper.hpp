#ifndef SERENITY_TESTS_USAGE_HELPER_HPP
#define SERENITY_TESTS_USAGE_HELPER_HPP

#include <string>

#include <gtest/gtest.h>

#include <mesos/mesos.pb.h>
#include <mesos/resources.hpp>

#include <process/future.hpp>

#include <pbjson.hpp>

#include <stout/os.hpp>

#include "json_source.pb.h"

namespace mesos {
namespace serenity {
namespace tests {

const std::string SERENITY_FIXTURES_DIR = "tests/fixtures/";

class JsonUsage
{
public:
  static const Try<FixtureResourceUsage> ReadJson(
      const std::string& relativePath)
  {
    Try<std::string> content = os::read(relativePath);
    if (content.isError()) {
      return Error("Read error: " + content.error());
    } else if (!content.isSome()){
      return Error("Readed file is none");
    }

    std::string err;
    FixtureResourceUsage usages;
    int reply = pbjson::json2pb(content.get(), &usages, err);
    if (reply != 0){
      Try<std::string> emsg = strings::format(
          "Error during json deserialization | errno: %d | err: %s", reply, err);
      return Error(emsg.get());
    }

    return usages;
  }
};


/**
 * Fake usage function (same method as in mesos::slave::Slave).
 * This is used only to test integration with mesos.
 * For internal or filter tests use JsonSource.
 */
class MockSlaveUsage
{
public:
  MockSlaveUsage(const std::string& jsonSource)
  {
    Try<mesos::FixtureResourceUsage> usages = JsonUsage::ReadJson(jsonSource);
    if (usages.isError()){
      LOG(ERROR) << "Json Usage failed: " << usages.error() << std::endl;
    }
    // Take first fixture only.
    results = usages.get().resource_usage(0);
  }

  process::Future<ResourceUsage> usage()
  {
    return results;
  }

private:
  ResourceUsage results;
};

} // namespace tests {
} // namespace serenity {
} // namespace mesos {

#endif //SERENITY_TESTS_USAGE_HELPER_HPP
