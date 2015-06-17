#include "tests/common/sources/json_source.hpp"

namespace mesos {
namespace serenity {
namespace tests {

void JsonSource::RunTests(const std::string& jsonSource)
{
  Try<mesos::FixtureResourceUsage> usages = JsonUsage::ReadJson(jsonSource);
  if (usages.isError()){
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  for(auto itr = usages.get().resource_usage().begin();
      itr != usages.get().resource_usage().end();
      itr++)
  {
    produce(*itr);
  }

  return;
}

} // namespace tests {
} // namespace serenity {
} // namespace mesos {

