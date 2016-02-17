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

const constexpr char* FIELD_INT = "FIELD_INT";
const constexpr int64_t DEFAULT_FIELD_INT = -435;
const constexpr int64_t MODIFIED_FIELD_INT = 34535;

const constexpr char* FIELD_DOUBLE = "FIELD_DOUBLE";
const constexpr double_t DEFAULT_FIELD_DOUBLE = 0.345345;
const constexpr double_t MODIFIED_FIELD_DOUBLE = 3.432;


SerenityConfig loadSampleConfig() {
  SerenityConfig config;
  config.set(FIELD_STR, (std::string) MODIFIED_FIELD_STR);
  config.set(FIELD_BOOL, MODIFIED_FIELD_BOOL);
  config.set(FIELD_INT, MODIFIED_FIELD_INT);
  config.set(FIELD_DOUBLE, MODIFIED_FIELD_DOUBLE);

  return config;
}

TEST(SerenityConfigTest, EmptyItemsTest) {
  SerenityConfig config;
  EXPECT_NONE(config.item<std::string>(FIELD_STR));
  EXPECT_NONE(config.item<bool>(FIELD_BOOL));
  EXPECT_NONE(config.item<int64_t>(FIELD_INT));
  EXPECT_NONE(config.item<double_t>(FIELD_DOUBLE));
}

TEST(SerenityConfigTest, DefaultItemsTest) {
  SerenityConfig config;
  EXPECT_EQ(config.item<std::string>(FIELD_STR, DEFAULT_FIELD_STR),
            DEFAULT_FIELD_STR);
  EXPECT_EQ(config.item<bool>(FIELD_BOOL, DEFAULT_FIELD_BOOL),
            DEFAULT_FIELD_BOOL);
  EXPECT_EQ(config.item<int64_t>(FIELD_INT, DEFAULT_FIELD_INT),
            DEFAULT_FIELD_INT);
  EXPECT_EQ(config.item<double_t>(FIELD_DOUBLE, DEFAULT_FIELD_DOUBLE),
            DEFAULT_FIELD_DOUBLE);
}


TEST(SerenityConfigTest, ModifiedItemsTest) {
  SerenityConfig config;
  EXPECT_EQ(config.item<std::string>(FIELD_STR, DEFAULT_FIELD_STR),
            DEFAULT_FIELD_STR);
  EXPECT_EQ(config.item<bool>(FIELD_BOOL, DEFAULT_FIELD_BOOL),
            DEFAULT_FIELD_BOOL);
  EXPECT_EQ(config.item<int64_t>(FIELD_INT, DEFAULT_FIELD_INT),
            DEFAULT_FIELD_INT);
  EXPECT_EQ(config.item<double_t>(FIELD_DOUBLE, DEFAULT_FIELD_DOUBLE),
            DEFAULT_FIELD_DOUBLE);

  config = loadSampleConfig();

  EXPECT_EQ(config.item<std::string>(FIELD_STR, DEFAULT_FIELD_STR),
            MODIFIED_FIELD_STR);
  EXPECT_EQ(config.item<bool>(FIELD_BOOL, DEFAULT_FIELD_BOOL),
            MODIFIED_FIELD_BOOL);
  EXPECT_EQ(config.item<int64_t>(FIELD_INT, DEFAULT_FIELD_INT),
            MODIFIED_FIELD_INT);
  EXPECT_EQ(config.item<double_t>(FIELD_DOUBLE, DEFAULT_FIELD_DOUBLE),
            MODIFIED_FIELD_DOUBLE);
}

TEST(SerenityConfigTest, ErrorItemsTest) {
  SerenityConfig config = loadSampleConfig();
  EXPECT_ERROR(config.item<bool>(FIELD_STR));
  EXPECT_ERROR(config.item<int64_t>(FIELD_BOOL));
  EXPECT_ERROR(config.item<double_t>(FIELD_INT));
  EXPECT_ERROR(config.item<std::string>(FIELD_DOUBLE));
}

}  // namespace tests
}  // namespace serenity
}  // namespace mesos
