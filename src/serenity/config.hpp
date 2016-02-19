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

template <typename T>
class SerenityItem {
 public:
  SerenityItem(T _value, T _defaultValue, std::string _key) :
    value(_value), defaultValue(_defaultValue), key(_key) {};

  T getValueOrDefault() {
    if (validationFailed) {
      return defaultValue;
    }

    return value;
  }

  //! Validates that value is below given threshold item.
  SerenityItem<T>& validateValueIsBelow(SerenityItem<T> _thresholdItem) {
    assertTypeMatchNumericVariant<T>();

    if (value > _thresholdItem.getValueOrDefault()) {
      LOG(WARNING) << key << " option which is " << value
                   << "must be below the " << _thresholdItem.key
                   << " which is " << _thresholdItem.getValueOrDefault();

      validationFailed = true;
    }

    return *this;
  }

  //! Validates that value is below given threshold.
  SerenityItem<T>& validateValueIsBelow(T thresholdValue) {
    assertTypeMatchNumericVariant<T>();

    if (value > thresholdValue) {
      LOG(WARNING) << key << " option which is " << value
                   << "must be below " << thresholdValue;

      validationFailed = true;
    }

    return *this;
  }

  //! Validates that value is above given threshold item.
  SerenityItem<T>& validateValueIsAbove(SerenityItem<T> _thresholdItem) {
    assertTypeMatchNumericVariant<T>();

    if (value < _thresholdItem.getValueOrDefault()) {
      LOG(WARNING) << key << " option which is " << value
      << "must be above the " << _thresholdItem.key
      << " which is " << _thresholdItem.getValueOrDefault();

      validationFailed = true;
    }

    return *this;
  }

  //! Validates that value is above given threshold.
  SerenityItem<T>& validateValueIsAbove(T thresholdValue) {
    assertTypeMatchNumericVariant<T>();

    if (value < thresholdValue) {
      LOG(WARNING) << key << " option which is " << value
      << "must be above " << thresholdValue;

      validationFailed = true;
    }

    return *this;
  }

  //! Validates that value is positive.
  SerenityItem<T>& validateValueIsPositive() {
    assertTypeMatchNumericVariant<T>();

    if (value < 0) {
      LOG(WARNING) << key << " option which is " << value
      << "must be above 0";

      validationFailed = true;
    }

    return *this;
  }

 protected:
  T value;
  const T defaultValue;
  const std::string key;
  bool validationFailed = false;

 private:
  template <typename T>
  static void assertTypeMatchNumericVariant() const {
    static_assert(std::is_same<T, int64_t>()
                  || std::is_same<T, double_t>(),
                  "Function supports only following numeric types: int64_t, "
                    "double_t");
  }
};


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
   * Safe getter for item in config.
   * Return default value in case of error or none.
   */
  template <typename T>
  const SerenityItem<T> getItemAndSetDefault(
    const std::string& key, T defaultValue) const {
    assertTypeMatchVariant<T>();

    T result = defaultValue;

    // Get item from items map.
    Option<SerenityConfig::Item> value = getVariantValue(key);

    if (value.isSome()) {
      // When item is found, try to parse it to the specified T type.
      try {
        result = boost::get<T>(value.get());
      } catch (std::exception& e) {
        LOG(ERROR) << "Failed to parse " << key << " to type "
                   << typeid(T).name() << ". Field: " << e.what();
      }
    }

    return SerenityItem<T>(result, defaultValue, key);
  }

  /**
   * Gets config section.
   */
  Option<SerenityConfig> getSection(const std::string& sectionKey) {
    auto mapItem = this->sections.find(sectionKey);
    if (mapItem != this->sections.end()) {
      return *(mapItem->second);
    }

    // Element not found.
    return None();
  }

  /**
   * Gets config section.
   * In case there is not one, create empty section.
   */
  SerenityConfig& getSectionOrNew(const std::string& sectionKey) {
    auto mapItem = this->sections.find(sectionKey);
    if (mapItem != this->sections.end()) {
      return *(mapItem->second);
    }

    // In case of no section under this key - create empty section.
    std::shared_ptr<SerenityConfig> newSection =
      std::shared_ptr<SerenityConfig>(new SerenityConfig());
    sections[sectionKey] = newSection;

    return *newSection;
  }

  bool hasKey(const std::string& key) const {
    return items.find(key) != items.end();
  }

  /**
  * Overlapping custom configuration options using recursive copy.
  */
  void applyConfig(const SerenityConfig& customCfg) {
    recursiveCfgCopy(this, customCfg);
  }

 protected:
  /**
   * Variant type for storing multiple types of data in configuration.
   */
  using Item = boost::variant<bool, int64_t, double_t, std::string>;

  /**
   * Item
   */
  std::unordered_map<std::string, SerenityConfig::Item> items;

  /**
   * Support for hierarchical configuration sections.
   */
  std::unordered_map<std::string, std::shared_ptr<SerenityConfig>> sections;

  /**
   * Put config value for Item types.
   */
  template <typename T>
  void put(const std::string& key, T value) {
    this->putVariant(key, value);
  }

  /**
   * Put config value for char*.
   */
  void put(const std::string& key, char* value) {
    this->putVariant(key, (std::string) value);
  }

  /**
   * Put Item config value.
   */
  void putVariant(const std::string& key, SerenityConfig::Item value) {
    this->items[key] = value;
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
          &(base->getSectionOrNew(customSection.first)), *customSection.second);
    }
  }

  /**
   * Getter for Item.
   */
  Option<SerenityConfig::Item> getVariantValue(
    const std::string& itemKey) const {
    auto mapItem = this->items.find(itemKey);
    if (mapItem != this->items.end()) {
      return mapItem->second;
    }

    // Element not found.
    return None();
  }

 private:
  template <typename T>
  static void assertTypeMatchVariant() const {
    static_assert(std::is_same<T, bool>()
                  || std::is_same<T, int64_t>()
                  || std::is_same<T, double_t>()
                  || std::is_same<T, std::string>(),
                  "Config supports only following types: bool, int64_t, "
                  "double_t, string");
  }
};


}  // namespace serenity
}  // namespace mesos


#endif  // SERENITY_CONFIG_HPP
