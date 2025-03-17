#ifndef TADAH_CORE_BLUEPRINT_PARSER_H
#define TADAH_CORE_BLUEPRINT_PARSER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <toml.hpp>  // toml11 v4.4.0
#include <boost/algorithm/string.hpp>
#include <cmrc/cmrc.hpp>

CMRC_DECLARE(CORE);

namespace tadah {
namespace core {

/**
 * @brief Structure describing one allowed key as defined in the blueprint.
 *
 * Contains the allowed data types, formatting instructions,
 * token count bounds, description, and additional metadata.
 */
struct BlueprintItem {
  std::string keyName;                     ///< Fully qualified key name (e.g., "ALLOWED_KEYS.config")
  std::vector<std::string> value_type;     ///< List of accepted types (e.g., "string", "int", "double", "Matrix", "v_type", "mixed")
  std::vector<std::string> value_format;   ///< Expected token format (e.g., "<file>", "<path>", "N", etc.)
  std::vector<int> value_N;                ///< Allowed token counts (for scalar or array values)
  std::string description;                 ///< Short description for documentation and help
  std::vector<std::string> examples;       ///< Example usages
  std::string defaultValue;                ///< Default value (if any)
  std::string long_option;                 ///< CLI long option (if applicable)
  std::string short_option;                ///< CLI short option (if applicable)
  std::vector<std::string> excludes;       ///< List of keys that must not coexist with this key
  bool required;                           ///< Indicates if required in a task context

  BlueprintItem() : required(false) {}
};

/**
 * @brief Parses a blueprint file (TOML) and expands INHERIT blocks.
 *
 * This class is responsible for reading the blueprint and producing a mapping
 * from key names to BlueprintItem structures. Any conflicts or missing information
 * result in an exception.
 */
class BlueprintParser {
public:
  /**
   * @brief Constructs a BlueprintParser.
   * @param blueprintFile Filename of the blueprint (TOML format)
   */
  explicit BlueprintParser(const std::string &blueprintFile);

  /**
   * @brief Constructs a BlueprintParser.
   *
   * Uses the default blueprint file "config/config_blueprint.toml".
   */
  BlueprintParser();

  /**
   * @brief Parses and expands the blueprint.
   *
   * This method reads the blueprint file, expands any INHERIT blocks,
   * and constructs a mapping of key names to BlueprintItem structures.
   * All keys are uppercased.
   *
   * Currently, the parser uses ALLOWED_KEYS only.
   *
   * @return An unordered_map mapping fully qualified key names to BlueprintItem.
   * @throws std::runtime_error on parsing errors or inheritance conflicts.
   */
  std::unordered_map<std::string, BlueprintItem> parse();

private:
  std::string blueprintFile_;
  toml::value blueprint_; ///< Raw parsed TOML data

  /**
   * @brief Recursively expands any INHERIT blocks in a TOML table.
   *
   * Merges inherited keys from parent scope.
   *
   * @param table The TOML table to process.
   * @param parentScope The dot-separated path of the current table.
   */
  void expandInheritance(toml::value &table, const std::string &parentScope = "");

  /**
   * @brief Retrieves a table from the TOML root using a dot-separated path.
   *
   * @param root The root TOML value.
   * @param path Dot-separated key path.
   * @return A reference to the requested TOML value.
   * @throws std::runtime_error if the path is invalid.
   */
  const toml::value& getTableByPath(const toml::value &root, const std::string &path) const;

  /**
   * @brief Merges keys from the source table into the destination table.
   *
   * Destination table values override those from the source.
   *
   * @param dest Destination TOML table.
   * @param source Source TOML table.
   */
  void mergeTables(toml::value &dest, const toml::value &source);

  /**
   * @brief Constructs a BlueprintItem from a TOML table.
   * @param fullKey Fully qualified key name.
   * @param table TOML table for the key.
   * @return A BlueprintItem representing the key's schema.
   */
  BlueprintItem buildBlueprintItem(const std::string &fullKey, const toml::value &table);
};

} // namespace core
} // namespace tadah

#endif // TADAH_CORE_BLUEPRINT_PARSER_H

