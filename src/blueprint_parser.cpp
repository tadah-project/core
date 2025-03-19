#include "tadah/core/blueprint_parser.h"
#include <algorithm>
#include <iostream>
#include <cmrc/cmrc.hpp>

CMRC_DECLARE(CORE);

namespace tadah {
namespace core {

namespace {  // Anonymous namespace for internal helper functions

  // CMRC filesystem blueprint file path.
  const std::string blueprintFilePath = "config/config_blueprint.toml";
/**
 * @brief Recursively expands any INHERIT blocks in a TOML table.
 *
 * Expands inheritance by merging keys from inherited tables (referenced by the "INHERIT" key)
 * into the current table. Processes subtables recursively. Handles nested inheritance
 * by recursively expanding each inherited table first, ensuring all inheritance chains
 * are processed.
 *
 * @param table TOML table to process (modified in place).
 * @param root The root TOML value used for lookup of inherited tables.
 * @param parentScope Dot-separated path representing the current scope (primarily for diagnostics).
 * @throws std::runtime_error If any errors occur during inheritance expansion.
 *
 * Example usage:
 * @code
 * toml::value config = toml::parse("input.toml");
 * expandInheritance(config, config);
 * // config now has all keys from inherited sections recursively merged
 * @endcode
 */
void expandInheritance(toml::value &table,
                       const toml::value &root,
                       const std::string &parentScope = "")
{
  // Process only if the current node is a table
  if (!table.is_table()) {
    return;
  }

  // Process all sub-keys recursively first, so they are fully expanded.
  for (auto &kv : table.as_table()) {
    if (kv.second.is_table()) {
      std::string newScope = parentScope.empty() ? kv.first : (parentScope + "." + kv.first);
      expandInheritance(kv.second, root, newScope);
    }
  }

  // Check if the current table has an INHERIT key
  if (table.contains("INHERIT")) {
    try {
      // Retrieve all mentioned paths in the INHERIT array
      auto inheritList = toml::find<std::vector<std::string>>(table, "INHERIT");

      // For each path, locate the base table in 'root' and merge it into 'table'
      for (const std::string &inheritPath : inheritList) {
        // Split the dot-separated path into individual parts
        std::vector<std::string> parts;
        boost::split(parts, inheritPath, boost::is_any_of("."));

        // Traverse the root using the path parts
        const toml::value *curr = &root;
        for (const std::string &part : parts) {
          if (!curr->is_table()) {
            throw std::runtime_error("Invalid inheritance path: " + inheritPath);
          }
          curr = &toml::find(*curr, part);
        }
        if (!curr->is_table()) {
          throw std::runtime_error("Inheritance source is not a table: " + inheritPath);
        }

        // Make a local copy of the inherited table so it can be expanded recursively
        toml::value expandedBase = *curr;
        expandInheritance(expandedBase, root, inheritPath);

        // Merge the fully expanded base table into the current table
        auto &baseTable = expandedBase.as_table();
        auto &destTable = table.as_table();
        for (auto &basePair : baseTable) {
          if (destTable.find(basePair.first) == destTable.end()) {
            destTable[basePair.first] = basePair.second;
          }
        }
      }
    } catch (const std::exception &ex) {
      throw std::runtime_error("Error during inheritance expansion: " + std::string(ex.what()));
    }

    // Remove the INHERIT key so it is not merged again
    table.as_table().erase("INHERIT");
  }
}

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
  /*void expandInheritance(toml::value &table,*/
  /*                       const toml::value &root,*/
  /*                       const std::string &parentScope = "")*/
  /*{*/
  /*  if (table.is_table()) {*/
  /*    // Process each sub-key recursively.*/
  /*    for (auto &kv : table.as_table()) {*/
  /*      if (kv.second.is_table()) {*/
  /*        std::string newScope = parentScope.empty() ? kv.first : (parentScope + "." + kv.first);*/
  /*        expandInheritance(kv.second, root, newScope);*/
  /*      }*/
  /*    }*/
  /*    // Process the INHERIT key if present.*/
  /*    if (table.contains("INHERIT")) {*/
  /*      try {*/
  /*        auto inheritList = toml::find<std::vector<std::string>>(table, "INHERIT");*/
  /*        for (const std::string &inheritPath : inheritList) {*/
  /*          // Perform lookup using dot-separated path.*/
  /*          std::vector<std::string> parts;*/
  /*          boost::split(parts, inheritPath, boost::is_any_of("."));*/
  /*          const toml::value *curr = &root;*/
  /*          for (const std::string &part : parts) {*/
  /*            if (!curr->is_table()) {*/
  /*              throw std::runtime_error("Invalid inheritance path: " + inheritPath);*/
  /*            }*/
  /*            curr = &toml::find(*curr, part);*/
  /*          }*/
  /*          // Merge inherited keys if not already defined.*/
  /*          const auto &baseTable = curr->as_table();*/
  /*          auto &destTable = table.as_table();*/
  /*          for (const auto &basePair : baseTable) {*/
  /*            if (destTable.find(basePair.first) == destTable.end()) {*/
  /*              destTable[basePair.first] = basePair.second;*/
  /*            }*/
  /*          }*/
  /*        }*/
  /*      } catch (const std::exception &ex) {*/
  /*        throw std::runtime_error("Error during inheritance expansion: " + std::string(ex.what()));*/
  /*      }*/
  /*      // Remove the INHERIT key after processing.*/
  /*      table.as_table().erase("INHERIT");*/
  /*    }*/
  /*  }*/
  /*}*/

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
      item.defaultValue = toml::find<std::vector<std::string>>(table, "default");
    if (table.contains("cli_long_flag"))
      item.cli_long_flag = toml::find<std::string>(table, "cli_long_flag");
    if (table.contains("cli_short_flag"))
      item.cli_short_flag = toml::find<std::string>(table, "cli_short_flag");
    if (table.contains("cli_excludes"))
      item.cli_excludes = toml::find<std::vector<std::string>>(table, "cli_excludes");
    if (table.contains("cli_required"))
      item.cli_required = toml::find<bool>(table, "cli_required");
    else
      item.cli_required = false;
    return item;
  }

  /**
   * @brief Builds the blueprint map from the TOML blueprint file.
   *
   * Parses and processes the TOML configuration file, applying inheritance expansion,
   * and then extracts the table to build the mapping of blueprint keys
   * to BlueprintItem objects.
   *
   * @return An unordered_map mapping blueprint key names to BlueprintItem objects.
   * @throws std::runtime_error If any error occurs during parsing or processing.
   */
  std::unordered_map<std::string, BlueprintItem> buildBlueprintMap(const std::string &table)
  {
    std::unordered_map<std::string, BlueprintItem> map;
    try {
      // Parse the blueprint file.
      auto fs = cmrc::CORE::get_filesystem();
      auto toml_file = fs.open(blueprintFilePath);
      std::stringstream ss;
      ss << toml_file.begin();
      toml::value blueprint = toml::parse(ss);
      // Create a copy to process inheritance.
      toml::value bpCopy = blueprint;
      // Recursively expand inheritance.
      expandInheritance(bpCopy, blueprint, "");
      // Locate the table.
      const auto &items = toml::find<toml::table>(bpCopy, table);
      // Process each item in the table.
      for (const auto &itemPair : items) {
        std::string key = itemPair.first;
        // Convert key to uppercase.
        std::transform(key.begin(), key.end(), key.begin(), ::toupper);
        map[key] = buildBlueprintItem(key, itemPair.second);
      }
    } catch (const std::exception &ex) {
      throw std::runtime_error("Error in blueprint map initialization: " + std::string(ex.what()));
    }
    return map;
  }

} // end anonymous namespace

///////////////////////////////////////////////////////////////////////////////
/// Retrieves a copy of the statically cached blueprint map.
/// @return A copy of the unordered_map mapping blueprint keys to BlueprintItem objects.
/// @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
///////////////////////////////////////////////////////////////////////////////
std::unordered_map<std::string, BlueprintItem> BlueprintParser::getKeysMap() {
  // Return a copy of the static blueprint map.
  return getKeysMapRef();
}

std::unordered_map<std::string, BlueprintItem> BlueprintParser::getInternalKeysMap() {
  // Return a copy of the static blueprint map.
  return getInternalKeysMapRef();
}

///////////////////////////////////////////////////////////////////////////////
/// Retrieves a const reference to the statically cached blueprint map.
/// Uses a static local variable to guarantee one-time initialization.
/// @return Const reference to the unordered_map mapping blueprint keys to BlueprintItem objects.
/// @throws std::runtime_error If blueprint parsing or inheritance expansion fails.
///////////////////////////////////////////////////////////////////////////////
const std::unordered_map<std::string, BlueprintItem>& BlueprintParser::getKeysMapRef() {
  static std::unordered_map<std::string, BlueprintItem> blueprintAllowedMap = buildBlueprintMap("ALLOWED_KEYS");
  return blueprintAllowedMap;
}
const std::unordered_map<std::string, BlueprintItem>& BlueprintParser::getInternalKeysMapRef() {
  static std::unordered_map<std::string, BlueprintItem> blueprintInternalMap = buildBlueprintMap("INTERNAL_KEYS");
  return blueprintInternalMap;
}

} // namespace core
} // namespace tadah

