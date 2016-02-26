#ifndef SERENITY_CONFIG_HPP
#define SERENITY_CONFIG_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <type_traits>

#include "boost/variant.hpp"

#include "serenity/serenity.hpp"

#include "stout/option.hpp"
#include "stout/result.hpp"

namespace mesos {
namespace serenity {


/**
 * Class which implement validation of the given value.
 * In case of
 */
template <typename T>
class ConfigValidator {
 public:
  explicit ConfigValidator(
      Result<T> _value, Option<std::string> _key = None())
    : value(_value), key(_key) {}

  T getOrElse(T defaultValue) {
    if (!value.isSome() || validationFailed) {
      return defaultValue;
    }

    return value.get();
  }


  //! Validates that value is below given threshold.
  ConfigValidator<T>& validateValueIsBelow(
      T thresholdValue, std::string additionalMsg = "") {
    assertTypeMatchNumericVariant();

    if (!value.isSome()) {
      return *this;
    }

    if (value.get() > thresholdValue) {
      LOG(WARNING) << key.getOrElse("") << " option which is " << value.get()
                   << "must be below " << thresholdValue << ". "
                   << additionalMsg;

      validationFailed = true;
    }

    return *this;
  }

  //! Validates that value is above given threshold.
  ConfigValidator<T>& validateValueIsAbove(
      T thresholdValue, std::string additionalMsg = "") {
    assertTypeMatchNumericVariant();

    if (!value.isSome()) {
      return *this;
    }

    if (value.get() < thresholdValue) {
      LOG(WARNING) << key.getOrElse("") << " option which is " << value.get()
                   << "must be above " << thresholdValue << ". "
                   << additionalMsg;

      validationFailed = true;
    }

    return *this;
  }

  //! Validates that value is positive.
  ConfigValidator<T>& validateValueIsPositive() {
    assertTypeMatchNumericVariant();

    if (!value.isSome()) {
      return *this;
    }

    if (value.get() < 0) {
      LOG(WARNING) << key.getOrElse("") << " option which is " << value.get()
      << "must be above 0";

      validationFailed = true;
    }

    return *this;
  }

 protected:
  Result<T> value;
  const Option<std::string> key;

  bool validationFailed = false;

 private:
  static void assertTypeMatchNumericVariant() {
    static_assert(std::is_same<T, int64_t>()
                  || std::is_same<T, double_t>(),
                  "Function supports only following numeric types: int64_t, "
                    "double_t");
  }
};


/**
 * Config class which implements basic mechanism
 * to support specifying config parameters via string key map & sections.
 *
 * Check config_test.cpp to see example usage.
 */
class Config {
 public:
  Config() {}

  /**
   * Getter for value in config.
   */
  template <typename T>
  const Result<T> getValue(const std::string& key) const {
    assertTypeMatchVariant<T>();

    Result<T> result = None();

    // Get item from items map.
    Option<Value> value = getVariantValue(key);

    if (value.isSome()) {
      // When item is found, try to parse it to the specified T type.
      try {
        result = boost::get<T>(value.get());
      } catch (std::exception& e) {
        std::stringstream ss;
        ss << "Failed to parse " << key << " to type "
           << typeid(T).name() << ". Field: " << e.what();
        LOG(ERROR) << ss.str();
        result = Error(ss.str());
      }
    }

    return result;
  }

  /**
   * Gets config section.
   */
  Option<Config> getSection(const std::string& sectionKey) {
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
  const Config& getSectionOrNew(const std::string& sectionKey) {
    auto mapItem = this->sections.find(sectionKey);
    if (mapItem != this->sections.end()) {
      return *(mapItem->second);
    }

    // In case of no section under this key - create empty section.
    std::shared_ptr<Config> newSection =
      std::shared_ptr<Config>(new Config());
    sections[sectionKey] = newSection;

    return *newSection;
  }

  bool hasKey(const std::string& key) const {
    return items.find(key) != items.end();
  }

  /**
  * Overlapping custom configuration options using recursive copy.
  */
  void applyConfig(const Config& customCfg) {
    recursiveCfgCopy(this, customCfg);
  }

 protected:
  /**
   * Variant type for storing multiple types of data in configuration.
   */
  using Value = boost::variant<bool, int64_t, double_t, std::string>;

  /**
   * Item
   */
  std::unordered_map<std::string, Value> items;

  /**
   * Support for hierarchical configuration sections.
   */
  std::unordered_map<std::string, std::shared_ptr<Config>> sections;

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
  void putVariant(const std::string& key, Value value) {
    this->items[key] = value;
  }

//  /**
//   * Put config section.
//   */
//  virtual void putSection(const std::string& sectionKey, Config config) {
//    this->sections[sectionKey] = config;
//  }

  /**
   * Recursive copy.
   */
  void recursiveCfgCopy(Config* base,
                        const Config& customCfg) const {
    for (auto customItem : customCfg.items) {
      base->items[customItem.first] = customItem.second;
    }

    for (auto customSection : customCfg.sections) {
      this->recursiveCfgCopy(
          &(base->getSectionRefOrNew(customSection.first)),
          *customSection.second);
    }
  }

  /**
   * Getter for Variant Value.
   */
  Option<Value> getVariantValue(
    const std::string& itemKey) const {
    auto mapItem = this->items.find(itemKey);
    if (mapItem != this->items.end()) {
      return mapItem->second;
    }

    // Element not found.
    return None();
  }

  /**
  * Gets config section.
  * In case there is not one, create empty section.
  */
  Config& getSectionRefOrNew(const std::string& sectionKey) {
    auto mapItem = this->sections.find(sectionKey);
    if (mapItem != this->sections.end()) {
      return *(mapItem->second);
    }

    // In case of no section under this key - create empty section.
    std::shared_ptr<Config> newSection =
      std::shared_ptr<Config>(new Config());
    sections[sectionKey] = newSection;

    return *newSection;
  }

 private:
  template <typename T>
  static void assertTypeMatchVariant() {
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
