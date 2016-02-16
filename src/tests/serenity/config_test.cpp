#include <string>

#include "gtest/gtest.h"
#include "serenity/config.hpp"

#include "stout/gtest.hpp"

#include "tests/common/config_helper.hpp"

namespace mesos {
namespace serenity {
namespace tests {

// TestConfig required items & default values using different types.
const constexpr char* FIELD_STR = "FIELD_STR";
const constexpr char* DEFAULT_FIELD_STR = "default";
const constexpr char* MODIFIED_FIELD_STR = "modified";

const constexpr char* FIELD_BOOL = "FIELD_BOOL";
const constexpr bool DEFAULT_FIELD_BOOL = true;
const constexpr bool MODIFIED_FIELD_BOOL = false;

const constexpr char* FIELD_UINT = "FIELD_UINT";
const constexpr uint64_t DEFAULT_FIELD_UINT = 23424;
const constexpr uint64_t MODIFIED_FIELD_UINT = 3;

const constexpr char* FIELD_INT = "FIELD_INT";
const constexpr int64_t DEFAULT_FIELD_INT = -435;
const constexpr int64_t MODIFIED_FIELD_INT = 34535;

const constexpr char* FIELD_DOUBLE = "FIELD_DOUBLE";
const constexpr double_t DEFAULT_FIELD_DOUBLE = 0.345345;
const constexpr double_t MODIFIED_FIELD_DOUBLE = 3.432;


class TestConfigFilter {
 public:
  explicit TestConfigFilter(const SerenityConfig& customCfg) {
    configure(customCfg);
  }

  void configure(const SerenityConfig& externalConf) {
    config = SerenityConfig();

    config.set(FIELD_STR, (std::string)DEFAULT_FIELD_STR);
    config.set(FIELD_BOOL, DEFAULT_FIELD_BOOL);
    config.set(FIELD_UINT, DEFAULT_FIELD_UINT);
    config.set(FIELD_INT, DEFAULT_FIELD_INT);
    config.set(FIELD_DOUBLE, DEFAULT_FIELD_DOUBLE);

    config.applyConfig(externalConf);
  }

  SerenityConfig config;
};


TEST(SerenityConfigTest, DefaultValuesAvailable) {
  // Create empty config with no configuration items.
  SerenityConfig newConfig;

  TestConfigFilter testFilter = TestConfigFilter(newConfig);
  EXPECT_EQ(testFilter.config.item<std::string>(FIELD_STR).get(),
            (std::string) DEFAULT_FIELD_STR);
  EXPECT_EQ(testFilter.config.item<bool>(FIELD_BOOL).get(),
            DEFAULT_FIELD_BOOL);
  EXPECT_EQ(testFilter.config.item<uint64_t>(FIELD_UINT).get(),
            DEFAULT_FIELD_UINT);
  EXPECT_EQ(testFilter.config.item<int64_t>(FIELD_INT).get(),
            DEFAULT_FIELD_INT);
  EXPECT_EQ(testFilter.config.item<double_t>(FIELD_DOUBLE).get(),
            DEFAULT_FIELD_DOUBLE);

  EXPECT_EQ(testFilter.config.item<bool>(FIELD_BOOL).get(), DEFAULT_FIELD_BOOL);
}


TEST(SerenityConfigTest, ModifiedValuesAvailable) {
  // Create config with custom configuration items.
  SerenityConfig newConfig;
  newConfig.set(FIELD_STR, (std::string)MODIFIED_FIELD_STR);
  newConfig.set(FIELD_BOOL, MODIFIED_FIELD_BOOL);
  newConfig.set(FIELD_UINT, MODIFIED_FIELD_UINT);
  newConfig.set(FIELD_INT, MODIFIED_FIELD_INT);
  newConfig.set(FIELD_DOUBLE, MODIFIED_FIELD_DOUBLE);

  TestConfigFilter testFilter = TestConfigFilter(newConfig);

  EXPECT_EQ(testFilter.config.item<std::string>(FIELD_STR).get(),
            (std::string) MODIFIED_FIELD_STR);
  EXPECT_EQ(testFilter.config.item<bool>(FIELD_BOOL).get(),
            MODIFIED_FIELD_BOOL);
  EXPECT_EQ(testFilter.config.item<uint64_t>(FIELD_UINT).get(),
            MODIFIED_FIELD_UINT);
  EXPECT_EQ(testFilter.config.item<int64_t>(FIELD_INT).get(),
            MODIFIED_FIELD_INT);
  EXPECT_EQ(testFilter.config.item<double_t>(FIELD_DOUBLE).get(),
            MODIFIED_FIELD_DOUBLE);
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos
