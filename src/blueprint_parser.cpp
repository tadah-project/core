#include "tadah/core/blueprint_parser.h"
#include "tadah/core/logger.h"
#include <algorithm>
#include <cmrc/cmrc.hpp>
#include <iostream>

CMRC_DECLARE(CORE);

namespace tadah {
namespace core {

namespace { // Anonymous namespace for internal helper functions

// CMRC filesystem blueprint file path.
const std::string blueprintFilePath = "config/config_blueprint.toml";
/**
 * @brief Recursively expands any INHERIT blocks in a TOML table.
 *
 * Expands inheritance by merging keys from inherited tables (referenced by the
 * "INHERIT" key) into the current table. Processes subtables recursively.
 * Handles nested inheritance by recursively expanding each inherited table
 * first, ensuring all inheritance chains are processed.
 *
 * @param table TOML table to process (modified in place).
 * @param root The root TOML value used for lookup of inherited tables.
 * @param parentScope Dot-separated path representing the current scope
 * (primarily for diagnostics).
 * @throws std::runtime_error If any errors occur during inheritance expansion.
 *
 * Example usage:
 * @code
 * toml::value config = toml::parse("input.toml");
 * expandInheritance(config, config);
 * // config now has all keys from inherited sections recursively merged
 * @endcode
 */
void expandInheritance(toml::value &table, const toml::value &root,
                       const std::string &parentScope = "") {
  // Process only if the current node is a table
  if (!table.is_table()) {
    return;
  }

  // Process all sub-keys recursively first, so they are fully expanded.
  for (auto &kv : table.as_table()) {
    if (kv.second.is_table()) {
      std::string newScope =
          parentScope.empty() ? kv.first : (parentScope + "." + kv.first);
      expandInheritance(kv.second, root, newScope);
    }
  }

  // Check if the current table has an INHERIT key
  if (table.contains("INHERIT")) {
    try {
      // Retrieve all mentioned paths in the INHERIT array
      auto inheritList = toml::find<std::vector<std::string>>(table, "INHERIT");

      // For each path, locate the base table in 'root' and merge it into
      // 'table'
      for (const std::string &inheritPath : inheritList) {
        // Split the dot-separated path into individual parts
        std::vector<std::string> parts;
        boost::split(parts, inheritPath, boost::is_any_of("."));

        // Traverse the root using the path parts
        const toml::value *curr = &root;
        for (const std::string &part : parts) {
          if (!curr->is_table()) {
            throw std::runtime_error("Invalid inheritance path: " +
                                     inheritPath);
          }
          curr = &toml::find(*curr, part);
        }
        if (!curr->is_table()) {
          throw std::runtime_error("Inheritance source is not a table: " +
                                   inheritPath);
        }

        // Make a local copy of the inherited table so it can be expanded
        // recursively
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
      throw std::runtime_error("Error during inheritance expansion: " +
                               std::string(ex.what()));
    }

    // Remove the INHERIT key so it is not merged again
    table.as_table().erase("INHERIT");
  }
}
/**
 * @brief Constructs a BlueprintItem from a TOML table.
 *
 * Validates and converts the given TOML subtable into a BlueprintItem
 * structure.
 *
 * value_type, value_format, and value_N are mandatory fields. A runtime_error
 * is thrown if any are missing. All other fields are optional and, if missing,
 * are either set to a default value or generate a warning.
 *
 * Example usage:
 * @code
 * toml::value table = toml::parse("some_file.toml");
 * BlueprintItem item = buildBlueprintItem("ALLOW_SOMETHING",
 * table["ALLOW_SOMETHING"]);
 * @endcode
 *
 * @param fullKey The fully qualified key name.
 * @param table Reference to the TOML subtable for the blueprint item.
 * @return A BlueprintItem constructed from the given TOML table.
 * @throws std::runtime_error If any mandatory field (value_type, value_format,
 * or value_N) is missing.
 */
BlueprintItem buildBlueprintItem(const std::string &fullKey,
                                 const toml::value &table) {
  BlueprintItem item;
  item.keyName = fullKey;

  // Throws an exception if 'value_type' is not found
  if (!table.contains("value_type")) {
    throw std::runtime_error("Missing mandatory field 'value_type' in key: " +
                             fullKey);
  }
  item.value_type = toml::find<std::vector<std::string>>(table, "value_type");

  // Throws an exception if 'value_format' is not found
  if (!table.contains("value_format")) {
    throw std::runtime_error("Missing mandatory field 'value_format' in key: " +
                             fullKey);
  }
  item.value_format =
      toml::find<std::vector<std::string>>(table, "value_format");

  // Throws an exception if 'value_N' is not found
  if (!table.contains("value_N")) {
    throw std::runtime_error("Missing mandatory field 'value_N' in key: " +
                             fullKey);
  }
  item.value_N = toml::find<std::vector<int>>(table, "value_N");

  // Warns if optional fields are missing
  try {
    item.description = toml::find<std::string>(table, "description");
    item.examples = toml::find<std::vector<std::string>>(table, "examples");
  } catch (const std::exception &ex) {
    Logger::instance().log(LogLevel::WARNING,
                           "Missing optional fields for key " + fullKey + ": " +
                               std::string(ex.what()));
  }

  // Handles other optional fields
  if (table.contains("default")) {
    item.defaultValue = toml::find<std::vector<std::string>>(table, "default");
  }
  if (table.contains("cli_long_flag")) {
    item.cli_long_flag = toml::find<std::string>(table, "cli_long_flag");
  }
  if (table.contains("cli_short_flag")) {
    item.cli_short_flag = toml::find<std::string>(table, "cli_short_flag");
  }
  if (table.contains("cli_excludes")) {
    item.cli_excludes =
        toml::find<std::vector<std::string>>(table, "cli_excludes");
  }
  if (table.contains("cli_required")) {
    item.cli_required = toml::find<bool>(table, "cli_required");
  } else {
    item.cli_required = false;
  }

  return item;
}

/**
 * @brief Builds the blueprint map from the TOML blueprint file.
 *
 * Parses and processes the TOML configuration file, applying inheritance
 * expansion, and then extracts the table to build the mapping of blueprint keys
 * to BlueprintItem objects.
 *
 * @return An unordered_map mapping blueprint key names to BlueprintItem
 * objects.
 * @throws std::runtime_error If any error occurs during parsing or processing.
 */
std::unordered_map<std::string, BlueprintItem>
buildBlueprintMap(const std::string &table) {
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
    throw std::runtime_error("Error in blueprint map initialization: " +
                             std::string(ex.what()));
  }
  return map;
}

} // end anonymous namespace

///////////////////////////////////////////////////////////////////////////////
/// Retrieves a copy of the statically cached blueprint map.
/// @return A copy of the unordered_map mapping blueprint keys to BlueprintItem
/// objects.
/// @throws std::runtime_error If blueprint parsing or inheritance expansion
/// fails.
///////////////////////////////////////////////////////////////////////////////
std::unordered_map<std::string, BlueprintItem> BlueprintParser::getKeysMap() {
  // Return a copy of the static blueprint map.
  return getKeysMapRef();
}

std::unordered_map<std::string, BlueprintItem>
BlueprintParser::getInternalKeysMap() {
  // Return a copy of the static blueprint map.
  return getInternalKeysMapRef();
}

///////////////////////////////////////////////////////////////////////////////
/// Retrieves a const reference to the statically cached blueprint map.
/// Uses a static local variable to guarantee one-time initialization.
/// @return Const reference to the unordered_map mapping blueprint keys to
/// BlueprintItem objects.
/// @throws std::runtime_error If blueprint parsing or inheritance expansion
/// fails.
///////////////////////////////////////////////////////////////////////////////
const std::unordered_map<std::string, BlueprintItem> &
BlueprintParser::getKeysMapRef() {
  static std::unordered_map<std::string, BlueprintItem> blueprintAllowedMap =
      buildBlueprintMap("ALLOWED_KEYS");
  return blueprintAllowedMap;
}
const std::unordered_map<std::string, BlueprintItem> &
BlueprintParser::getInternalKeysMapRef() {
  static std::unordered_map<std::string, BlueprintItem> blueprintInternalMap =
      buildBlueprintMap("INTERNAL_KEYS");
  return blueprintInternalMap;
}

std::string BlueprintParser::findClosestKey(
    const std::string &key,
    const std::unordered_map<std::string, BlueprintItem> &map) {
  // Early return if the map is empty
  if (map.empty()) {
    return "";
  }

  // Helper function computing Levenshtein distance using dynamic programming
  auto computeLevenshteinDistance = [](const std::string &s1,
                                       const std::string &s2) {
    const std::size_t m = s1.size();
    const std::size_t n = s2.size();

    // Vectors store the cost for transforming prefixes up to the current
    // character
    std::vector<int> prevRow(n + 1), currRow(n + 1);

    // Initial boundary conditions for s1= empty prefix to full length
    for (std::size_t j = 0; j <= n; ++j) {
      prevRow[j] = static_cast<int>(j);
    }

    // Fill the DP table row by row
    for (std::size_t i = 0; i < m; ++i) {
      currRow[0] = static_cast<int>(i + 1);
      for (std::size_t j = 0; j < n; ++j) {
        // Cost of substituting s1[i] with s2[j]
        int cost = (s1[i] == s2[j]) ? 0 : 1;

        // Compute minimal cost from neighboring states
        currRow[j + 1] = std::min(std::min(prevRow[j + 1] + 1, // Deletion
                                           currRow[j] + 1),    // Insertion
                                  prevRow[j] + cost            // Substitution
        );
      }
      // Swap the current row into previous to reuse storage
      prevRow.swap(currRow);
    }
    // After filling the table, the last cell of prevRow holds the edit distance
    return prevRow[n];
  };

  int bestDistance = std::numeric_limits<int>::max();
  std::string bestKey;

  // Loop over all keys in the map, computing the edit distance and tracking the
  // minimal result
  for (const auto &pair : map) {
    int dist = computeLevenshteinDistance(key, pair.first);
    if (dist < bestDistance) {
      bestDistance = dist;
      bestKey = pair.first;
    }
  }

  return bestKey;
}
} // namespace core
} // namespace tadah
