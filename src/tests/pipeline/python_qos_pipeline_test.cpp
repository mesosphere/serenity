#include <list>

#include "gtest/gtest.h"

#include "mesos/slave/oversubscription.hpp"

#include "messages/serenity.hpp"

#include "pipeline/python_qos_pipeline.hpp"

#include "stout/gtest.hpp"

#include "tests/common/config_helper.hpp"
#include "tests/common/usage_helper.hpp"

namespace mesos {
namespace serenity {
namespace tests {

const constexpr char* TEST_PYPELINE_PATH = "tests/python/";

TEST(PythonQoSPipelineTest, RunPythonTestPipeline) {
  uint64_t WINDOWS_SIZE = 10;
  uint64_t CONTENTION_COOLDOWN = 10;
  double_t FRATIONAL_THRESHOLD = 0.5;

  Try<mesos::FixtureResourceUsage> usages =
      JsonUsage::ReadJson("tests/fixtures/pipeline/insufficient_metrics.json");
  if (usages.isError()) {
    LOG(ERROR) << "JsonSource failed: " << usages.error() << std::endl;
  }

  ResourceUsage usage;
  usage.CopyFrom(usages.get().resource_usage(0));

  SerenityConfig conf;
  conf[PypelineFilter::NAME].set(SERENITY_PYPELINE_PATH,
                                 (std::string) TEST_PYPELINE_PATH);

  double onEmptyCorrectionInterval = 2;

  QoSControllerPipeline* pipeline = new PythonQoSPipeline(conf);

  Result<QoSCorrections> corrections = pipeline->run(usage);

  EXPECT_NONE(corrections);

  delete pipeline;
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

