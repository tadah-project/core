#include "tadah/core/blueprint_parser.h"
#include <algorithm>
#include <iostream>
#include <cmrc/cmrc.hpp>

CMRC_DECLARE(CORE);

namespace tadah {
namespace core {

namespace {  // Anonymous namespace for internal helper functions

  // Hardcoded blueprint file path.
  const std::string blueprintFilePath = "/home/s1351949/PROJECTS/TADAH/CORE/config/config_blueprint.toml";

  /**
   * @brief Recursively expands any INHERIT blocks in a TOML table.
   *
   * Expands inheritance by merging keys from inherited tables (referenced by the "INHERIT" key)
   * into the current table. Processes subtables recursively.
   *
   * @param table TOML table to process (modified in place).
   * @param root The root TOML value used for lookup of inherited tables.
   * @param parentScope Dot-separated path representing the current scope.
   * @throws std::runtime_error If any errors occur during inheritance expansion.
   */
  void expandInheritance(toml::value &table,
                         const toml::value &root,
                         const std::string &parentScope = "")
  {
    if (table.is_table()) {
      // Process each sub-key recursively.
      for (auto &kv : table.as_table()) {
        if (kv.second.is_table()) {
          std::string newScope = parentScope.empty() ? kv.first : (parentScope + "." + kv.first);
          expandInheritance(kv.second, root, newScope);
        }
      }
      // Process the INHERIT key if present.
      if (table.contains("INHERIT")) {
        try {
          auto inheritList = toml::find<std::vector<std::string>>(table, "INHERIT");
          for (const std::string &inheritPath : inheritList) {
            // Perform lookup using dot-separated path.
            std::vector<std::string> parts;
            boost::split(parts, inheritPath, boost::is_any_of("."));
            const toml::value *curr = &root;
            for (const std::string &part : parts) {
              if (!curr->is_table()) {
                throw std::runtime_error("Invalid inheritance path: " + inheritPath);
              }
              curr = &toml::find(*curr, part);
            }
            // Merge inherited keys if not already defined.
            const auto &baseTable = curr->as_table();
            auto &destTable = table.as_table();
            for (const auto &basePair : baseTable) {
              if (destTable.find(basePair.first) == destTable.end()) {
                destTable[basePair.first] = basePair.second;
              }
            }
          }
        } catch (const std::exception &ex) {
          throw std::runtime_error("Error during inheritance expansion: " + std::string(ex.what()));
        }
        // Remove the INHERIT key after processing.
        table.as_table().erase("INHERIT");
      }
    }
  }

  /**
   * @brief Constructs a BlueprintItem from a TOML table.
   *
   * Validates and converts the given TOML subtable into a BlueprintItem structure.
   *
   * @param fullKey Fully qualified key name.
   * @param table TOML subtable corresponding to the blueprint item.
   * @return Constructed BlueprintItem.
   * @throws std::runtime_error If mandatory fields are missing or of incorrect type.
   */
  BlueprintItem buildBlueprintItem(const std::string &fullKey,
                                   const toml::value &table)
  {
    BlueprintItem item;
    item.keyName = fullKey;
    try {
      item.value_type   = toml::find<std::vector<std::string>>(table, "value_type");
      item.value_format = toml::find<std::vector<std::string>>(table, "value_format");
      item.value_N      = toml::find<std::vector<int>>(table, "value_N");
      item.description  = toml::find<std::string>(table, "description");
      item.examples     = toml::find<std::vector<std::string>>(table, "examples");
    } catch (const std::exception &ex) {
      std::cerr << "Warning: Missing fields for key " << fullKey << ": " << ex.what() << std::endl;
    }
    if (table.contains("default"))
      item.defaultValue = toml::find<std::string>(table, "default");
    if (table.contains("long_option"))
      item.long_option = toml::find<std::string>(table, "long_option");
    if (table.contains("short_option"))
      item.short_option = toml::find<std::string>(table, "short_option");
    if (table.contains("excludes"))
      item.excludes = toml::find<std::vector<std::string>>(table, "excludes");
    if (table.contains("required"))
      item.required = toml::find<bool>(table, "required");
    else
      item.required = false;
    return item;
  }

  /**
   * @brief Builds the blueprint map from the TOML blueprint file.
   *
   * Parses and processes the TOML configuration file, applying inheritance expansion,
   * and then extracts the "ALLOWED_KEYS" table to build the mapping of blueprint keys
   * to BlueprintItem objects.
   *
   * @return An unordered_map mapping blueprint key names to BlueprintItem objects.
   * @throws std::runtime_error If any error occurs during parsing or processing.
   */
  std::unordered_map<std::string, BlueprintItem> buildBlueprintMap()
  {
    std::unordered_map<std::string, BlueprintItem> map;
    try {
      // Parse the blueprint file.
      toml::value blueprint = toml::parse(blueprintFilePath);
      // Create a copy to process inheritance.
      toml::value bpCopy = blueprint;
      // Recursively expand inheritance.
      expandInheritance(bpCopy, blueprint, "");
      // Locate the ALLOWED_KEYS table.
      const auto &allowed = toml::find<toml::table>(bpCopy, "ALLOWED_KEYS");
      // Process each item in the ALLOWED_KEYS table.
      for (const auto &itemPair : allowed) {
        std::string key = itemPair.first;
        // Convert key to uppercase.
        std::transform(key.begin(), key.end(), key.begin(), ::toupper);
        map[key] = buildBlueprintItem(key, itemPair.second);
      }
    } catch (const std::exception &ex) {
      throw std::runtime_error("Error in blueprint map initialization: " + std::string(ex.what()));
    }
    std::cout << "Blueprint map successfully built (" << map.size() << " entries)." << std::endl;
    return map;
  }

} // end anonymous namespace

///////////////////////////////////////////////////////////////////////////////
/// Retrieves a copy of the statically cached blueprint map.
/// @return A copy of the unordered_map mapping blueprint keys to BlueprintItem objects.
/// @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
///////////////////////////////////////////////////////////////////////////////
std::unordered_map<std::string, BlueprintItem> BlueprintParser::getMap() {
  // Return a copy of the static blueprint map.
  return getMapRef();
}

///////////////////////////////////////////////////////////////////////////////
/// Retrieves a const reference to the statically cached blueprint map.
/// Uses a static local variable to guarantee one-time initialization.
/// @return Const reference to the unordered_map mapping blueprint keys to BlueprintItem objects.
/// @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
///////////////////////////////////////////////////////////////////////////////
const std::unordered_map<std::string, BlueprintItem>& BlueprintParser::getMapRef() {
  static std::unordered_map<std::string, BlueprintItem> blueprintMap = buildBlueprintMap();
  return blueprintMap;
}

} // namespace core
} // namespace tadah

