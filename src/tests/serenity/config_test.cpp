#include <string>

#include "gtest/gtest.h"
#include "serenity/config.hpp"

#include "stout/gtest.hpp"

#include "tests/common/config_helper.hpp"

namespace mesos {
namespace serenity {
namespace tests {

// TestConfig required fields & default values using different types.
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


class TestConfig : public SerenityConfig {
 public:
  TestConfig() {
    this->initDefaults();
  }

  /**
   * This constructor enables run-time overlapping of default
   * configuration records.
   */
  explicit TestConfig(const SerenityConfig& customCfg) {
    this->initDefaults();
    this->cpy(customCfg);
  }

  /**
   * Init default values for Test configuration.
   */
  void initDefaults() {
    this->set(FIELD_STR, (std::string)DEFAULT_FIELD_STR);
    this->set(FIELD_BOOL, DEFAULT_FIELD_BOOL);
    this->set(FIELD_UINT, DEFAULT_FIELD_UINT);
    this->set(FIELD_INT, DEFAULT_FIELD_INT);
    this->set(FIELD_DOUBLE, DEFAULT_FIELD_DOUBLE);
  }
};


TEST(SerenityConfigTest, DefaultValuesAvailable) {
  // Create empty config with no configuration fields.
  SerenityConfig newConfig;

  TestConfig internalConfig = TestConfig(newConfig);
  EXPECT_EQ(internalConfig.getS(FIELD_STR), (std::string)DEFAULT_FIELD_STR);
  EXPECT_EQ(internalConfig.getB(FIELD_BOOL), DEFAULT_FIELD_BOOL);
  EXPECT_EQ(internalConfig.getU64(FIELD_UINT), DEFAULT_FIELD_UINT);
  EXPECT_EQ(internalConfig.getI64(FIELD_INT), DEFAULT_FIELD_INT);
  EXPECT_EQ(internalConfig.getD(FIELD_DOUBLE), DEFAULT_FIELD_DOUBLE);
}


TEST(SerenityConfigTest, ModifiedValuesAvailable) {
  // Create config with custom configuration fields.
  SerenityConfig newConfig;
  newConfig.set(FIELD_STR, (std::string)MODIFIED_FIELD_STR);
  newConfig.set(FIELD_BOOL, MODIFIED_FIELD_BOOL);
  newConfig.set(FIELD_UINT, MODIFIED_FIELD_UINT);
  newConfig.set(FIELD_INT, MODIFIED_FIELD_INT);
  newConfig.set(FIELD_DOUBLE, MODIFIED_FIELD_DOUBLE);

  SerenityConfig internalConfig = TestConfig(newConfig);
  EXPECT_EQ(internalConfig.getS(FIELD_STR), (std::string)MODIFIED_FIELD_STR);
  EXPECT_EQ(internalConfig.getB(FIELD_BOOL), MODIFIED_FIELD_BOOL);
  EXPECT_EQ(internalConfig.getU64(FIELD_UINT), MODIFIED_FIELD_UINT);
  EXPECT_EQ(internalConfig.getI64(FIELD_INT), MODIFIED_FIELD_INT);
  EXPECT_EQ(internalConfig.getD(FIELD_DOUBLE), MODIFIED_FIELD_DOUBLE);
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos

