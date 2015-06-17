#ifndef SERENITY_TEST_JSON_SOURCE_HPP
#define SERENITY_TEST_JSON_SOURCE_HPP

#include <glog/logging.h>

#include <mesos/mesos.hpp>

#include "json_source.pb.h"

#include "serenity/serenity.hpp"

#include "tests/common/usage_helper.hpp"

namespace mesos {
namespace serenity {
namespace tests {

/*
 * Source for feeding pipeline with fixtured ResourceUsage
 */
class JsonSource : public Producer<ResourceUsage>
{
public:
  void RunTests(const std::string& jsonSource);
};

} // namespace tests {
} // namespace serenity {
} // namespace mesos {

#endif //SERENITY_TEST_JSON_SOURCE_HPP
