#ifndef SERENITY_CONFIG_HPP
#define SERENITY_CONFIG_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include "boost/variant.hpp"

#include "serenity/default_vars.hpp"
#include "serenity/serenity.hpp"

#include "stout/option.hpp"
#include "stout/result.hpp"

namespace mesos {
namespace serenity {

/**
 * Global Serenity Config class which implements basic mechanism
 * to support specifying config parameters via string key map & sections.
 *
 * Check config_test.cpp to see example usage.
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
    recursiveCfgCopy(this, customCfg);
  }

  /**
   * Templated, safe getter for item in config.
   */
  template <typename T>
  const Result<T> item(std::string key) const {
    Result<T> result = None();

    // Get item from items map.
    Option<SerenityConfig::CfgVariant> variantResult = getItem(key);

    if (variantResult.isSome()) {
      // When item is found, try to parse it to the specified T type.
      try {
        result = boost::get<T>(variantResult.get());
      } catch (std::exception& e) {
        // NOTE(bplotka): Log here????
        LOG(ERROR) << "Failed to parse " << key
        << " field: " << e.what();
        result = Result<T>::error(e.what());
      }
    }

    return result;
  }

  /**
   * Gets config section.
   * In case there is not one, create empty section.
   */
  SerenityConfig& operator[](std::string key) {
    return *getSection(key);
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
    this->items[key] = value;
  }

  bool hasKey(std::string key) const {
    return items.find(key) != items.end();
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
  std::unordered_map<std::string, SerenityConfig::CfgVariant> items;

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
  Option<SerenityConfig::CfgVariant> getItem(std::string itemKey) const {
    auto mapItem = this->items.find(itemKey);
    if (mapItem != this->items.end()) {
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
    for (auto customItem : customCfg.items) {
      base->items[customItem.first] = customItem.second;
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
