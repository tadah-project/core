#ifndef TADAH_CORE_BLUEPRINT_PARSER_H
#define TADAH_CORE_BLUEPRINT_PARSER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <toml.hpp>  // toml11 v4.4.0

namespace tadah {
namespace core {

/**
 * @brief Structure describing one allowed key as defined in the blueprint.
 *
 * Contains allowed data types, formatting instructions, token count bounds,
 * description, and additional metadata.
 */
struct BlueprintKey {
  std::string keyName;                            ///< Fully qualified key name (e.g., "ALLOWED_KEYS.CONFIG")
  std::vector<std::string> value_type;            ///< List of accepted types (e.g., "STRING", "INT", "DOUBLE", etc.)
  std::vector<std::string> value_format;          ///< Expected token format (e.g., "<file>", "<path>", "N", etc.)
  std::vector<int> value_N;                       ///< Allowed token counts (for scalar or array values)
  std::string description;                        ///< Short description for help
  std::string longDescription;                    ///< Long description for documentation and help
  std::vector<std::string> formatting;            ///< Output formatting options
  std::vector<std::string> examples;              ///< Example usages
  std::vector<std::string> default_value;         ///< Default value (if any)
  std::string cli_long_flag;                      ///< CLI long flag/option (if applicable)
  std::string cli_short_flag;                     ///< CLI short flag/option (if applicable)
  std::vector<std::string> cli_excludes;          ///< List of CLI keys/options that must not coexist with this key
  std::vector<std::string> cli_needs;             ///< List of CLI keys/options that are required for this CLI option
  bool cli_required;                              ///< Indicates if required in a cli context
  /**
   * Non-empty for a deprecated key: why it is deprecated and what replaces
   * it (blueprint field `deprecated`). A configuration or potential file may
   * still contain the key; its value is checked against the key's format,
   * then CoreUserConfigParser writes a notice to standard error once per
   * parse and does not store it.
   * A deprecated key has no default_value (Context::contains() would insert
   * it), no CLI flag and no examples.
   */
  std::string deprecated;
  /**
   * The only values a STRING key takes (blueprint field `allowed_values`),
   * matched without regard to case; empty for any value of its type.
   * CoreUserConfigParser rejects any other value at the line that sets it
   * ("Invalid value for key K on line N of <file>: 'v' (allowed: A, B)";
   * without "of <file>" for lines that come from no file), and
   * validateWithBlueprint -- the CLI and task path -- refuses it too. The
   * key's default_value and examples must be allowed values.
   */
  std::vector<std::string> allowed_values;

  BlueprintKey() : cli_required(false) {}

  /// Whether `value` is one of allowed_values (without regard to case), or
  /// the key has no list.
  bool isAllowedValue(const std::string &value) const;

  /// True if the blueprint marks this key as deprecated.
  bool isDeprecated() const { return !deprecated.empty(); }
};
struct BlueprintKeyGroup {
  std::string groupName;                          ///< Name of the group 
  std::vector<std::string> description;           ///< Description of the group
  std::vector<BlueprintKey> blueprintKeys;        ///< BlueprintKey objects in this group
  bool cli_required;                              ///< Indicates if required in a cli context
  size_t requiredMin;                             ///< Minimum number of required keys which must be selected
  size_t requiredMax;                             ///< Maximum number of required keys which must be selected
  std::vector<std::string> cli_excludes;          ///< List of CLI keys/options that must not coexist with this key
  std::vector<std::string> cli_needs;             ///< List of CLI keys/options that are required for this CLI option
  BlueprintKeyGroup() : cli_required(false), requiredMin(1), requiredMax(1) {}
};

struct BlueprintTask {
  std::string taskName;                     ///< The task's name, e.g., "TASK_A".
  std::string description;                  ///< Short description (mandatory).
  std::string longDescription;              ///< Extended description (mandatory).
  std::vector<std::string> examples;        ///< Usage examples (mandatory).
  std::vector<BlueprintKey> blueprintKeys;  ///< Multiple key-value pairs (optional).
  std::vector<BlueprintKeyGroup> groups;    ///< Key groups (optional).

  BlueprintTask() {}
};

struct BlueprintTadah: public BlueprintTask {
  std::string logo;                         ///< Name of the Tadah! software.
  std::string version;                      ///< Version of the Tadah! software.
  std::string website;                      ///< Version of the Tadah! software.
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
   * @return Const reference to the unordered_map mapping blueprint keys to BlueprintKey objects.
   * @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
   */
  static const std::unordered_map<std::string, BlueprintKey>& getKeysMapRef();

  /**
   * @brief Retrieves a const reference to the statically cached TASKS blueprint map.
   *
   * Access does not cause a copy and the map is built on first invocation.
   *
   * @return Const reference to the unordered_map mapping task names to BlueprintKey objects.
   * @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
   */
  static const std::unordered_map<std::string, BlueprintTask>& getTasksMapRef();

  /**
   * @brief Retrieves a const reference to the statically cached INTERNAL_KEYS blueprint map.
   *
   * Access does not cause a copy and the map is built on first invocation.
   *
   * @return Const reference to the unordered_map mapping internal keys to BlueprintKey objects.
   * @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
   */
  static const std::unordered_map<std::string, BlueprintKey>& getInternalKeysMapRef();

  /**
   * @brief Retrieves a copy of the statically cached ALLOWED_KEYS blueprint map.
   *
   * @return A copy of the unordered_map mapping blueprint keys to BlueprintKey objects.
   * @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
   */
  static std::unordered_map<std::string, BlueprintKey> getKeysMap();

  /**
   * @brief Retrieves a copy of the statically cached TASKS blueprint map.
   *
   * @return A copy of the unordered_map mapping blueprint keys to BlueprintKey objects.
   * @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
   */
  static std::unordered_map<std::string, BlueprintTask> getTasksMap();

  /**
   * @brief Retrieves a copy of the statically cached blueprint map.
   *
   * @return A copy of the BlueprintTadah object containing the blueprint data.
   * @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
   */
  static BlueprintTadah getBlueprintTadah();

  /**
   * @brief Retrieves a const reference to the statically cached blueprint map.
   *
   * Access does not cause a copy and the map is built on first invocation.
   *
   * @return Const reference to the BlueprintTadah object containing the blueprint data.
   * @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
   */
  static const BlueprintTadah &getBlueprintTadahRef();

  /**
   * @brief Retrieves a copy of the statically cached INTERNAL_KEYS blueprint map.
   *
   * @return A copy of the unordered_map mapping blueprint keys to BlueprintKey objects.
   * @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
   */
  static std::unordered_map<std::string, BlueprintKey> getInternalKeysMap();

  /**
   * @brief Builds one BlueprintKey from its (inheritance-expanded) TOML table.
   *
   * This is the constructor used for every key of the embedded blueprint;
   * it is public so that the per-key rules can be checked directly.
   * value_type, value_format and value_N are mandatory. A `deprecated`
   * field must be a non-empty string, and a deprecated key must define
   * neither `default_value`, a CLI flag nor examples (after inheritance:
   * `examples = []` overrides those of a TYPES table).
   *
   * @param fullKey Fully qualified key name (used in error messages).
   * @param table TOML table of the key.
   * @throws std::runtime_error If a mandatory field is missing or a rule is
   *         violated.
   */
  static BlueprintKey buildKey(const std::string &fullKey,
                               const toml::value &table);

  /**
 * @brief Computes the closest matching key name by measuring the edit distance to each valid blueprint key.
 *
 * The method calculates the Levenshtein distance between the provided key and each key in the map, returning
 * the one with the smallest distance. Deprecated keys are skipped. This behavior helps guide users toward valid keys when typos or
 * similar mistakes are made in configuration files or command-line input.
 *
 * @param key The user-provided (potentially erroneous) key.
 * @param map The map of valid key names associated with BlueprintKey objects.
 * @return The key name with the smallest edit distance, or an empty string if the map holds no
 *         key that is not deprecated.
 *
 * Example usage:
 * @code
 *   std::unordered_map<std::string, BlueprintKey> blueprintMap = {{"ALLOWED_KEYS", {}}};
 *   std::string closest = BlueprintParser::findClosestKey("ALOWED_KEYS", blueprintMap);
 *   // 'closest' would likely be "ALLOWED_KEYS"
 * @endcode
 */
  static std::string findClosestKey(const std::string &key,
      const std::unordered_map<std::string, BlueprintKey> &map);
};

} // namespace core
} // namespace tadah

#endif // TADAH_CORE_BLUEPRINT_PARSER_H
