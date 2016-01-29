#ifndef SERENITY_CONFIG_HPP
#define SERENITY_CONFIG_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include "boost/variant.hpp"

#include "serenity/default_vars.hpp"
#include "serenity/serenity.hpp"

#include "stout/option.hpp"

namespace mesos {
namespace serenity {

/**
 * Global Serenity Config class which implements basic mechanism
 * to support specifying config parameters via string key map.
 *
 * Check config_test.cpp to see example usage.
 *
 * TODO(skonefal): every getter should pack result in Try<T>.
 */
class SerenityConfig {
 public:
  SerenityConfig() {}

  /**
  * Variant type for storing multiple types of data in configuration.
  */
  using CfgVariant = boost::variant<
    bool, int64_t, uint64_t, double_t, std::string>;

  /**
   * Overlapping custom configuration options using recursive copy.
   */
  void applyConfig(const SerenityConfig& customCfg) {
    this->recursiveCfgCopy(this, customCfg);
  }

  /**
   * Gets variant config value.
   */
  Option<SerenityConfig::CfgVariant> operator()(std::string key) const {
    return getField(key);
  }

  /**
   * Gets config section.
   * In case there is not one, create empty section.
   */
  SerenityConfig& operator[](std::string key) {
    return *getSection(key);
  }

  // -- unsafe getters --

  /**
   * Unsafe getter for string
   */
  std::string getS(std::string key) {
    return boost::get<std::string>(this->fields[key]);
  }

  /**
   * Unsafe getter for int64_t
   */
  int64_t getI64(std::string key) {
    return boost::get<int64_t>(this->fields[key]);
  }

  /**
   * Unsafe getter for uint64_t
   */
  uint64_t getU64(std::string key) {
    return boost::get<uint64_t>(this->fields[key]);
  }

  /**
   * Unsafe getter for double_t
   */
  double_t getD(std::string key) {
    return boost::get<double_t>(this->fields[key]);
  }

  /**
   * Unsafe getter for bool
   */
  bool getB(std::string key) {
    return boost::get<bool>(this->fields[key]);
  }

  // -- setters --

  /**
   * Sets char* config value.
   */
  void set(std::string key, char* value) {
    this->setVariant(key, (std::string)value);
  }

  /**
   * Sets string config value.
   */
  void set(std::string key, std::string value) {
    this->setVariant(key, value);
  }

  /**
   * Sets bool config value.
   */
  void set(std::string key, bool value) {
    this->setVariant(key, value);
  }

  /**
   * Sets uint64_t config value.
   */
  void set(std::string key, uint64_t value) {
    this->setVariant(key, value);
  }

  /**
   * Sets int64_t config value.
   */
  void set(std::string key, int64_t value) {
    this->setVariant(key, value);
  }

  /**
   * Sets double_t config value.
   */
  void set(std::string key, double_t value) {
    this->setVariant(key, value);
  }

  /**
   * Sets CfgVariant config value.
   */
  void setVariant(std::string key, SerenityConfig::CfgVariant value) {
    this->fields[key] = value;
  }

  bool hasKey(std::string key) {
    return fields.find(key) != fields.end();
  }

  /**
   * TODO(skonefal): Add UT for usage of this enum.
   */
  enum ConfigurationType : int {
    BOOL = 0,
    INT64 = 1,
    UINT64 = 2,
    DOUBLE = 3,
    STRING = 4
  };

 protected:
  std::unordered_map<std::string, SerenityConfig::CfgVariant> fields;

  /**
   * Support for hierarchical configuration sections.
   */
  std::unordered_map<std::string, std::shared_ptr<SerenityConfig>> sections;

  /**
   * Getter for section.
   * In case of no section - create such.
   */
  std::shared_ptr<SerenityConfig> getSection(std::string sectionKey) {
    auto mapItem = this->sections.find(sectionKey);
    if (mapItem != this->sections.end()) {
      return mapItem->second;
    }

    // In case of no section under this key - create empty section.
    auto newSection = std::make_shared<SerenityConfig>(SerenityConfig());
    this->sections[sectionKey] = newSection;

    return newSection;
  }

  /**
   * Getter for field.
   */
  Option<SerenityConfig::CfgVariant> getField(std::string fieldKey) const {
    auto mapItem = this->fields.find(fieldKey);
    if (mapItem != this->fields.end()) {
      return mapItem->second;
    }

    // Element not found.
    return None();
  }

  /**
   * Recursive copy.
   */
  void recursiveCfgCopy(SerenityConfig* base,
                        const SerenityConfig& customCfg) const {
    for (auto customItem : customCfg.fields) {
      base->fields[customItem.first] = customItem.second;
    }

    for (auto customSection : customCfg.sections) {
      this->recursiveCfgCopy(
        &((*base)[customSection.first]), *customSection.second);
    }
  }
};


}  // namespace serenity
}  // namespace mesos


#endif  // SERENITY_CONFIG_HPP
