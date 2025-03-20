#ifndef TADAH_CORE_BLUEPRINT_PARSER_H
#define TADAH_CORE_BLUEPRINT_PARSER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <toml.hpp>  // toml11 v4.4.0
#include <boost/algorithm/string.hpp>

namespace tadah {
namespace core {

/**
 * @brief Structure describing one allowed key as defined in the blueprint.
 *
 * Contains allowed data types, formatting instructions, token count bounds,
 * description, and additional metadata.
 */
struct BlueprintItem {
  std::string keyName;                     ///< Fully qualified key name (e.g., "ALLOWED_KEYS.CONFIG")
  std::vector<std::string> value_type;     ///< List of accepted types (e.g., "STRING", "INT", "DOUBLE", etc.)
  std::vector<std::string> value_format;   ///< Expected token format (e.g., "<file>", "<path>", "N", etc.)
  std::vector<int> value_N;                ///< Allowed token counts (for scalar or array values)
  std::string description;                 ///< Short description for documentation and help
  std::vector<std::string> examples;       ///< Example usages
  std::vector<std::string> defaultValue;                ///< Default value (if any)
  std::string cli_long_flag;                   ///< CLI long flag/option (if applicable)
  std::string cli_short_flag;                  ///< CLI short flag/option (if applicable)
  std::vector<std::string> cli_excludes;       ///< List of CLI keys/options that must not coexist with this key
  bool cli_required;                           ///< Indicates if required in a cli task context

  BlueprintItem() : cli_required(false) {}
};

/**
 * @brief Provides access to the static blueprint map.
 *
 * This module builds the blueprint map only once using the hardcoded blueprint file,
 * and subsequent calls return either a const reference (to avoid copying) or a copy.
 *
 * The blueprint is parsed from a TOML file and processed for any inheritance blocks.
 *
 * Usage example:
 * @code
 *   const auto &blueprint = tadah::core::BlueprintParser::getKeysMapRef();
 *   // Use the blueprint for further processing.
 * @endcode
 */
class BlueprintParser {
public:
  /**
   * @brief Retrieves a const reference to the statically cached ALLOWED_KEYS blueprint map.
   *
   * Access does not cause a copy and the map is built on first invocation.
   *
   * @return Const reference to the unordered_map mapping blueprint keys to BlueprintItem objects.
   * @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
   */
  static const std::unordered_map<std::string, BlueprintItem>& getKeysMapRef();

  /**
   * @brief Retrieves a const reference to the statically cached INTERNAL_KEYS blueprint map.
   *
   * Access does not cause a copy and the map is built on first invocation.
   *
   * @return Const reference to the unordered_map mapping internal keys to BlueprintItem objects.
   * @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
   */
  static const std::unordered_map<std::string, BlueprintItem>& getInternalKeysMapRef();

  /**
   * @brief Retrieves a copy of the statically cached ALLOWED_KEYS blueprint map.
   *
   * @return A copy of the unordered_map mapping blueprint keys to BlueprintItem objects.
   * @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
   */
  static std::unordered_map<std::string, BlueprintItem> getKeysMap();

  /**
   * @brief Retrieves a copy of the statically cached INTERNAL_KEYS blueprint map.
   *
   * @return A copy of the unordered_map mapping blueprint keys to BlueprintItem objects.
   * @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
   */
  static std::unordered_map<std::string, BlueprintItem> getInternalKeysMap();

  /**
 * @brief Computes the closest matching key name by measuring the edit distance to each valid blueprint key.
 *
 * The method calculates the Levenshtein distance between the provided key and each key in the map, returning
 * the one with the smallest distance. This behavior helps guide users toward valid keys when typos or
 * similar mistakes are made in configuration files or command-line input.
 *
 * @param key The user-provided (potentially erroneous) key.
 * @param map The map of valid key names associated with BlueprintItem objects.
 * @return The key name with the smallest edit distance or an empty string if the map is empty.
 *
 * Example usage:
 * @code
 *   std::unordered_map<std::string, BlueprintItem> blueprintMap = {{"ALLOWED_KEYS", {}}};
 *   std::string closest = BlueprintParser::findClosestKey("ALOWED_KEYS", blueprintMap);
 *   // 'closest' would likely be "ALLOWED_KEYS"
 * @endcode
 */
  static std::string findClosestKey(const std::string &key,
      const std::unordered_map<std::string, BlueprintItem> &map);
};

} // namespace core
} // namespace tadah

#endif // TADAH_CORE_BLUEPRINT_PARSER_H
