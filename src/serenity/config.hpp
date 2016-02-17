#ifndef SERENITY_CONFIG_HPP
#define SERENITY_CONFIG_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <type_traits>

#include "boost/variant.hpp"

#include "serenity/default_vars.hpp"
#include "serenity/serenity.hpp"

#include "stout/option.hpp"
#include "stout/result.hpp"

namespace mesos {
namespace serenity {

/**
 * Serenity Config class which implements basic mechanism
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
    bool, int64_t, double_t, std::string>;

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
  const Result<T> item(const std::string& key) const {
    static_assert(std::is_same<T, bool>()
                  || std::is_same<T, int64_t>()
                  || std::is_same<T, double_t>()
                  || std::is_same<T, std::string>(),
                  "T must be one of the types stored in CfgVariant (bool, "
                  "int64_t, double_t, string)");

    Result<T> result = None();

    // Get item from items map.
    Option<SerenityConfig::CfgVariant> variantResult = getItem(key);

    if (variantResult.isSome()) {
      // When item is found, try to parse it to the specified T type.
      try {
        result = boost::get<T>(variantResult.get());
      } catch (std::exception& e) {
        LOG(ERROR) << "Failed to parse " << key
                   << " field: " << e.what();
        result = Result<T>::error(e.what());
      }
    }

    return result;
  }

  /**
   * Templated, safe getter for item in config. Sets default value in case of
   * error or none.
   */
  template <typename T>
  const T item(const std::string& key, T defaultValue) const {
    static_assert(std::is_same<T, bool>()
                  || std::is_same<T, int64_t>()
                  || std::is_same<T, double_t>()
                  || std::is_same<T, std::string>(),
                  "T must be one of the types stored in CfgVariant (bool, "
                    "int64_t, double_t, string)");
    T result = defaultValue;

    // Get item from items map.
    Option<SerenityConfig::CfgVariant> variantResult = getItem(key);

    if (variantResult.isSome()) {
      // When item is found, try to parse it to the specified T type.
      try {
        result = boost::get<T>(variantResult.get());
      } catch (std::exception& e) {
        LOG(ERROR) << "Failed to parse " << key << " to type "
                   << typeid(T).name() << ". Field: " << e.what();
      }
    }

    return result;
  }

  /**
   * Gets config section.
   * In case there is not one, create empty section.
   */
  SerenityConfig& operator[](const std::string& key) {
    return *getSection(key);
  }

  /**
   * Templated set config value.
   */
  template <typename T>
  void set(const std::string& key, T value) {
    static_assert(std::is_same<T, bool>()
                  || std::is_same<T, int64_t>()
                  || std::is_same<T, double_t>()
                  || std::is_same<T, std::string>(),
                  "T must be one of the types stored in CfgVariant (bool, "
                    "int64_t, double_t, string)");
    this->setVariant(key, value);
  }

  /**
   * Templated set config value for char*.
   */
  void set(const std::string& key, char* value) {
    this->setVariant(key, (std::string) value);
  }

  /**
   * Sets CfgVariant config value.
   */
  void setVariant(const std::string& key, SerenityConfig::CfgVariant value) {
    this->items[key] = value;
  }

  bool hasKey(const std::string& key) const {
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
  std::shared_ptr<SerenityConfig> getSection(const std::string& sectionKey) {
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
  Option<SerenityConfig::CfgVariant> getItem(const std::string& itemKey) const {
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
