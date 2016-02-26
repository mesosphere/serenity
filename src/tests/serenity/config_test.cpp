#include <string>

#include "gtest/gtest.h"
#include "serenity/config.hpp"

#include "stout/gtest.hpp"

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

class TestConfig : Config {
 public:
  void loadSampleConfig() {
    put(FIELD_STR, (std::string) MODIFIED_FIELD_STR);
    put(FIELD_BOOL, MODIFIED_FIELD_BOOL);
    put(FIELD_INT, MODIFIED_FIELD_INT);
    put(FIELD_DOUBLE, MODIFIED_FIELD_DOUBLE);
  }

  void loadSampleConfigWithSections(std::string sectionName) {
    TestConfig config;
    config.put(FIELD_STR, (std::string) MODIFIED_FIELD_STR);
    config.put(FIELD_BOOL, MODIFIED_FIELD_BOOL);
    config.put(FIELD_INT, MODIFIED_FIELD_INT);
    config.put(FIELD_DOUBLE, MODIFIED_FIELD_DOUBLE);

    Config& config2 = getSectionRefOrNew(sectionName);
    config2.applyConfig(config);
  }

  /**
   * Put config value for Item types.
   */
  template <typename T>
  void put(const std::string& key, T value) {
    Config::putVariant(key, value);
  }

  /**
   * Getter for value in config.
   */
  template <typename T>
  const Result<T> getValue(const std::string& key) const {
    return Config::getValue<T>(key);
  }

  const Config& getSectionOrNew(const std::string& sectionKey) {
    return Config::getSectionOrNew(sectionKey);
  }
};

TEST(ConfigTest, EmptyItemsTest) {
  Config config;
  EXPECT_NONE(config.getValue<std::string>(FIELD_STR));
  EXPECT_NONE(config.getValue<bool>(FIELD_BOOL));
  EXPECT_NONE(config.getValue<int64_t>(FIELD_INT));
  EXPECT_NONE(config.getValue<double_t>(FIELD_DOUBLE));
}

TEST(ConfigTest, DefaultItemsTest) {
  Config config;
  EXPECT_EQ(ConfigValidator<std::string>(
      config.getValue<std::string>(FIELD_STR)).getOrElse(DEFAULT_FIELD_STR),
          DEFAULT_FIELD_STR);
  EXPECT_EQ(ConfigValidator<bool>(
    config.getValue<bool>(FIELD_BOOL)).getOrElse(DEFAULT_FIELD_BOOL),
            DEFAULT_FIELD_BOOL);
  EXPECT_EQ(ConfigValidator<int64_t>(
    config.getValue<int64_t>(FIELD_INT)).getOrElse(DEFAULT_FIELD_INT),
            DEFAULT_FIELD_INT);
  EXPECT_EQ(ConfigValidator<double_t>(
    config.getValue<double_t>(FIELD_DOUBLE)).getOrElse(DEFAULT_FIELD_DOUBLE),
            DEFAULT_FIELD_DOUBLE);
}


TEST(ConfigTest, ModifiedItemsTest) {
  TestConfig config;
  EXPECT_EQ(ConfigValidator<std::string>(
    config.getValue<std::string>(FIELD_STR)).getOrElse(DEFAULT_FIELD_STR),
            DEFAULT_FIELD_STR);
  EXPECT_EQ(ConfigValidator<bool>(
    config.getValue<bool>(FIELD_BOOL)).getOrElse(DEFAULT_FIELD_BOOL),
            DEFAULT_FIELD_BOOL);
  EXPECT_EQ(ConfigValidator<int64_t>(
    config.getValue<int64_t>(FIELD_INT)).getOrElse(DEFAULT_FIELD_INT),
            DEFAULT_FIELD_INT);
  EXPECT_EQ(ConfigValidator<double_t>(
    config.getValue<double_t>(FIELD_DOUBLE)).getOrElse(DEFAULT_FIELD_DOUBLE),
            DEFAULT_FIELD_DOUBLE);

  config.loadSampleConfig();

  EXPECT_EQ(ConfigValidator<std::string>(
    config.getValue<std::string>(FIELD_STR)).getOrElse(DEFAULT_FIELD_STR),
            MODIFIED_FIELD_STR);
  EXPECT_EQ(ConfigValidator<bool>(
    config.getValue<bool>(FIELD_BOOL)).getOrElse(DEFAULT_FIELD_BOOL),
            MODIFIED_FIELD_BOOL);
  EXPECT_EQ(ConfigValidator<int64_t>(
    config.getValue<int64_t>(FIELD_INT)).getOrElse(DEFAULT_FIELD_INT),
            MODIFIED_FIELD_INT);
  EXPECT_EQ(ConfigValidator<double_t>(
    config.getValue<double_t>(FIELD_DOUBLE)).getOrElse(DEFAULT_FIELD_DOUBLE),
            MODIFIED_FIELD_DOUBLE);
}

TEST(ConfigTest, ErrorItemsTest) {
  TestConfig config;
  config.loadSampleConfig();
  EXPECT_ERROR(config.getValue<bool>(FIELD_STR));
  EXPECT_ERROR(config.getValue<int64_t>(FIELD_BOOL));
  EXPECT_ERROR(config.getValue<double_t>(FIELD_INT));
  EXPECT_ERROR(config.getValue<std::string>(FIELD_DOUBLE));
}

TEST(ConfigTest, ModifiedSectionItemsTest) {
  const constexpr char* FIELD_SECTION = "SECTION1";
  TestConfig config;
  const Config& section = config.getSectionOrNew(FIELD_SECTION);

  EXPECT_EQ(ConfigValidator<std::string>(
    section.getValue<std::string>(FIELD_STR)).getOrElse(DEFAULT_FIELD_STR),
            DEFAULT_FIELD_STR);
  EXPECT_EQ(ConfigValidator<bool>(
    section.getValue<bool>(FIELD_BOOL)).getOrElse(DEFAULT_FIELD_BOOL),
            DEFAULT_FIELD_BOOL);
  EXPECT_EQ(ConfigValidator<int64_t>(
    section.getValue<int64_t>(FIELD_INT)).getOrElse(DEFAULT_FIELD_INT),
            DEFAULT_FIELD_INT);
  EXPECT_EQ(ConfigValidator<double_t>(
    section.getValue<double_t>(FIELD_DOUBLE)).getOrElse(DEFAULT_FIELD_DOUBLE),
            DEFAULT_FIELD_DOUBLE);

  config.loadSampleConfigWithSections(FIELD_SECTION);

  EXPECT_EQ(ConfigValidator<std::string>(
    section.getValue<std::string>(FIELD_STR)).getOrElse(DEFAULT_FIELD_STR),
            MODIFIED_FIELD_STR);
  EXPECT_EQ(ConfigValidator<bool>(
    section.getValue<bool>(FIELD_BOOL)).getOrElse(DEFAULT_FIELD_BOOL),
            MODIFIED_FIELD_BOOL);
  EXPECT_EQ(ConfigValidator<int64_t>(
    section.getValue<int64_t>(FIELD_INT)).getOrElse(DEFAULT_FIELD_INT),
            MODIFIED_FIELD_INT);
  EXPECT_EQ(ConfigValidator<double_t>(
    section.getValue<double_t>(FIELD_DOUBLE)).getOrElse(DEFAULT_FIELD_DOUBLE),
            MODIFIED_FIELD_DOUBLE);
}


}  // namespace tests
}  // namespace serenity
}  // namespace mesos
