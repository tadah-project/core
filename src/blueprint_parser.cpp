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
const std::string blueprintFilePath = "blueprint/blueprint_tadah.toml";
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
 * @brief Constructs a BlueprintKey from a TOML table.
 *
 * Validates and converts the given TOML subtable into a BlueprintKey
 * structure.
 *
 * value_type, value_format, and value_N are mandatory fields. A runtime_error
 * is thrown if any are missing. All other fields are optional and, if missing,
 * are either set to a default value or generate a warning.
 *
 * Example usage:
 * @code
 * toml::value table = toml::parse("some_file.toml");
 * BlueprintKey item = buildBlueprintKey("ALLOW_SOMETHING",
 * table["ALLOW_SOMETHING"]);
 * @endcode
 *
 * @param fullKey The fully qualified key name.
 * @param table Reference to the TOML subtable for the blueprint item.
 * @return A BlueprintKey constructed from the given TOML table.
 * @throws std::runtime_error If any mandatory field (value_type, value_format,
 * or value_N) is missing.
 */
BlueprintKey buildBlueprintKey(const std::string &fullKey,
                                 const toml::value &table) {
  BlueprintKey item;
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
 * to BlueprintKey objects.
 *
 * @return An unordered_map mapping blueprint key names to BlueprintKey
 * objects.
 * @throws std::runtime_error If any error occurs during parsing or processing.
 */
std::unordered_map<std::string, BlueprintKey>
buildBlueprintMap(const std::string &table) {
  std::unordered_map<std::string, BlueprintKey> map;
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
      map[key] = buildBlueprintKey(key, itemPair.second);
    }
  } catch (const std::exception &ex) {
    throw std::runtime_error("Error in blueprint map initialization: " +
                             std::string(ex.what()));
  }
  return map;
}


///////////////////////////////////////////////////////////////////////////////
// TASKS blueprint
///////////////////////////////////////////////////////////////////////////////
/**
 * @brief Handles a single-level "keys" subtable by splitting multi-path INHERIT 
 *        into per-key subtables, ensuring user-defined entries won't be overwritten.
 *
 * If [someTask.keys] has INHERIT = ["ALLOWED_KEYS.a", "ALLOWED_KEYS.b"] plus a 
 * subtable [someTask.keys.a] with existing fields, those fields are kept, and 
 * we add "ALLOWED_KEYS.a" to [someTask.keys.a].INHERIT so that a subsequent 
 * expandInheritance call merges only the missing fields from ALLOWED_KEYS.a. 
 * 
 * Behavior:
 *  - If a subtable "keys.<lastPart>" already exists, append the inheritance path
 *    to its existing array named INHERIT (creating that array if none exists).
 *  - If "keys.<lastPart>" does not yet exist, create it with INHERIT=[inheritPath].
 *  - Remove the original [someTask.keys].INHERIT so it's not processed again.
 *
 * @param table TOML subtree (often [TASKS.someTask]) in which to look 
 *              for a "keys" subtable at this level.
 */
static void prepareKeysInheritanceOneLevelWithMerge(toml::value &table) {
  if (!table.is_table()) {
    return;
  }

  // Traverse all child entries
  for (auto &kv : table.as_table()) {
    if (kv.first == "keys" && kv.second.is_table()) {
      toml::value &keysTbl = kv.second; // [someTask.keys]
      if (keysTbl.contains("INHERIT")) {
        // e.g. INHERIT = ["ALLOWED_KEYS.dbfile", "ALLOWED_KEYS.structure", ...]
        std::vector<std::string> inheritList =
          toml::find<std::vector<std::string>>(keysTbl, "INHERIT");

        for (const auto &inheritPath : inheritList) {
          // Use the last token as the subtable name. For "ALLOWED_KEYS.structure", 
          // we call it "structure".
          std::vector<std::string> parts;
          boost::split(parts, inheritPath, boost::is_any_of("."));
          if (parts.empty()) {
            throw std::runtime_error("Empty inheritance path encountered.");
          }
          const std::string lastName = parts.back();

          // Check if [keys.<lastName>] already exists
          auto &keysTableAsTable = keysTbl.as_table();
          auto found = keysTableAsTable.find(lastName);
          if (found != keysTableAsTable.end()) {
            // Subtable already exists. Append inheritPath to its INHERIT array.
            if (!found->second.is_table()) {
              throw std::runtime_error(
                "'keys." + lastName + "' is not a table but found under 'keys'."
              );
            }
            toml::table &existingSubtable = found->second.as_table();

            // If subtable doesn’t have an INHERIT array, create one.
            if (!existingSubtable.count("INHERIT")) {
              existingSubtable["INHERIT"] = toml::value(std::vector<std::string>());
            }
            // Append
            toml::value &existingInheritVal = existingSubtable["INHERIT"];
            if (!existingInheritVal.is_array()) {
              throw std::runtime_error(
                "'keys." + lastName + ".INHERIT' exists but is not an array."
              );
            }
            existingInheritVal.as_array().push_back(toml::value(inheritPath));
          } else {
            // Subtable [keys.<lastName>] does not exist yet, create it
            toml::table t;
            t["INHERIT"] = toml::value(std::vector<std::string>{ inheritPath });
            keysTbl[lastName] = toml::value(t);
          }
        }
        // Erase the original top-level INHERIT from [keys]
        keysTbl.as_table().erase("INHERIT");
      }
    }
    else if (kv.second.is_table()) {
      // Recurse deeper into any other table but skip if it's "keys"
      // to ensure we only process first-level "keys" table
      prepareKeysInheritanceOneLevelWithMerge(kv.second);
    }
  }
}

/**
 * @brief Helper that parses a single group's "keys" subtable,
 *        similarly to how parseTaskRec handles normal keys.
 */
static void parseGroupKeys(const toml::value &keysNode, std::vector<tadah::core::BlueprintKey> &outKeys, const std::string &parentGroup) {
  using namespace tadah::core;

  if (!keysNode.is_table()) {
    throw std::runtime_error("[groups] 'keys' must be a table in group: " + parentGroup);
  }

  // Each field in 'keysNode' might use an INHERIT array or define fields directly
  for (const auto &kv : keysNode.as_table()) {
    const std::string localKeyName = kv.first;  // e.g. "dbfile"
    std::string dottedKeyName = parentGroup + "." + localKeyName;

    // buildBlueprintKey or handle INHERIT:
    BlueprintKey newKey = buildBlueprintKey(dottedKeyName, kv.second);
    outKeys.push_back(newKey);
  }
}

/**
 * @brief Parse the [TASKS.<task>.groups] block, storing the results in a vector
 *        of BlueprintKeyGroup.
 */
static void parseGroups(const toml::value &groupsTbl,
                        std::vector<tadah::core::BlueprintKeyGroup> &outGroups,
                        const std::string &fullTaskName) {

  if (!groupsTbl.is_table()) {
    // "groups" block is not a table => ignore or throw
    throw std::runtime_error(fullTaskName + " has 'groups' but it is not a table.");
  }

  for (const auto &groupPair : groupsTbl.as_table()) {
    tadah::core::BlueprintKeyGroup group;
    group.groupName = groupPair.first; // e.g. "input"

    const toml::value &groupNode = groupPair.second;
    if (!groupNode.is_table()) {
      throw std::runtime_error("Group '" + group.groupName + "' under '" + fullTaskName + "' must be a table.");
    }

    // 1) description
    if (groupNode.contains("description")) {
      // could be string or array of strings. If it's a single string, store in group.description[0].
      const toml::value &descVal = groupNode.at("description");
      if (descVal.is_string()) {
        group.description.push_back(descVal.as_string());
      }
      else if (descVal.is_array()) {
        for (auto &elem : descVal.as_array()) {
          group.description.push_back(elem.as_string());
        }
      }
    }

    // 2) required_min
    if (groupNode.contains("required_min")) {
      group.requiredMin = toml::find<size_t>(groupNode, "required_min");
    }
    // 3) required_max
    if (groupNode.contains("required_max")) {
      group.requiredMax = toml::find<size_t>(groupNode, "required_max");
    }
    // 4) cli_required
    if (groupNode.contains("cli_required")) {
      group.cli_required = toml::find<bool>(groupNode, "cli_required");
    }
    // 5) cli_excludes (if you need group-level excludes)
    if (groupNode.contains("cli_excludes")) {
      group.cli_excludes = toml::find<std::vector<std::string>>(groupNode, "cli_excludes");
    }

    // 6) The [TASKS.<task>.groups.<groupName>.keys] subtable
    if (groupNode.contains("keys")) {
      parseGroupKeys(groupNode.at("keys"), group.blueprintKeys, fullTaskName + "." + group.groupName);
    }

    // Done
    outGroups.push_back(group);
  }
}


/**
 * @brief Helper function that parses a TOML value and (if valid) constructs a BlueprintTask.
 *
 * If the table includes the mandatory fields 'description', 'long_description', and 'examples',
 * the function creates a BlueprintTask. It also checks for a "keys" subtable to parse
 * multiple BlueprintKey objects. Any optional nested subtables (e.g., "twobody") are 
 * recursively processed as sub-tasks of the current task. Each nested sub-task has its 
 * dotted name (e.g. "PROPERTIES.TWOBODY").
 *
 * @param fullTaskName Dot-separated task name (e.g. "PROPERTIES.TWOBODY").
 * @param tomlNode TOML subtable representing the current task or sub-task.
 * @param outTasks A map that stores the final tasks by their dot-separated names.
 *
 * Example usage:
 * @code
 * std::unordered_map<std::string, BlueprintTask> tasks;
 * parseTaskRec("PROPERTIES", tomlSubtable, tasks);
 * @endcode
 */
static void parseTaskRec(const std::string &fullTaskName,
                         const toml::value &tomlNode,
                         std::unordered_map<std::string, BlueprintTask> &outTasks) {
  // Attempt to parse mandatory fields to decide if it's a valid 'task' block
  // Note: if the subtable does not contain these, it is either not a valid task 
  // (throw) or a purely parent container with no direct commands (can skip or handle differently).
  bool hasDescription      = tomlNode.contains("description");
  bool hasLongDescription  = tomlNode.contains("long_description");
  bool hasExamples         = tomlNode.contains("examples");

  // If all mandatory fields are present, parse them
  // Otherwise, either treat it as a non-task container or throw an exception
  if (hasDescription && hasLongDescription && hasExamples) {
    BlueprintTask newTask;
    newTask.taskName       = fullTaskName;
    newTask.description    = toml::find<std::string>(tomlNode, "description");
    newTask.longDescription = toml::find<std::string>(tomlNode, "long_description");
    newTask.examples       = toml::find<std::vector<std::string>>(tomlNode, "examples");

    // If there's a 'keys' subtable, parse blueprint keys.
    // Typically: [TASKS.properties.twobody.keys]
    if (tomlNode.contains("keys")) {
      const toml::value keysNode = toml::find(tomlNode, "keys");
      if (!keysNode.is_table()) {
        throw std::runtime_error("'keys' must be a table for task: " + fullTaskName);
      }
      // Iterate over each key entry (e.g., potential, outfile, range, ...)
      for (const auto &kv : keysNode.as_table()) {
        const std::string localKeyName = kv.first;
        // Build a fully qualified key name for debugging/trace
        std::string dottedKeyName = fullTaskName + "." + localKeyName;
        newTask.blueprintKeys.push_back(buildBlueprintKey(dottedKeyName, kv.second));
      }
    }

    if (tomlNode.contains("groups")) {
      const toml::value &groupsNode = tomlNode.at("groups");
      parseGroups(groupsNode, newTask.groups, fullTaskName);
    }

    // Store the parsed task in the map
    // Key = dot-separated name, value = the BlueprintTask object
    outTasks[fullTaskName] = std::move(newTask);
  }
  else {
    // If partial fields are present, throw with more information
    // (Alternatively, treat it as a container only if the user wants partial tasks to be ignored)
    if (hasDescription || hasLongDescription || hasExamples) {
      throw std::runtime_error("Task '" + fullTaskName +
        "' is missing one of the mandatory fields: description, long_description, or examples.");
    }
  }

  // Now check for deeper subcommands in the current node
  // We skip known fields (description, long_description, examples, keys and groups) and see if the child is a table.
  if (tomlNode.is_table()) {
  for (const auto &child : tomlNode.as_table()) {
    const std::string &k = child.first;

    // Skip any keys that are not sub-tasks
    if (k == "description" || k == "long_description" || k == "examples" ||
        k == "keys"        || k == "groups") {
      continue;
    }

    if (child.second.is_table()) {
      std::string subCommandName = fullTaskName.empty() ? k : (fullTaskName + "." + k);
      parseTaskRec(subCommandName, child.second, outTasks);
    }
  }
}
}

/**
 * @brief Builds the blueprint map for tasks from the TOML blueprint file,
 *        handling nested (dot-separated) sub-commands.
 *
 * - Reads the file from CMRC.
 * - Expands inheritance for each table.
 * - Locates the "TASKS" subtable to parse top-level and nested tasks.
 * - Calls parseTaskRec(...) to handle each subtable recursively.
 *
 * @param rootTable The top-level key ("TASKS" in your example).
 * @return An unordered_map mapping dot-separated subcommand names to BlueprintTask objects.
 * @throws std::runtime_error If parsing fails or mandatory fields are missing.
 */
static std::unordered_map<std::string, BlueprintTask>
buildBlueprintTasksMap(const std::string &rootTable) {
  std::unordered_map<std::string, BlueprintTask> tasksMap;

  try {
    // Parse blueprint file from embedded resource (CMRC).
    auto fs = cmrc::CORE::get_filesystem();
    auto toml_file = fs.open(blueprintFilePath);
    std::stringstream ss;
    ss << toml_file.begin();
    toml::value blueprint = toml::parse(ss);

    // Make a copy for safe expansion
    toml::value bpCopy = blueprint;
    prepareKeysInheritanceOneLevelWithMerge(bpCopy);
    expandInheritance(bpCopy, blueprint, "");

    // Locate the top-level tasks subtable (e.g. "TASKS")
    // e.g. [TASKS.transform], [TASKS.properties], ...
    const auto &tasksRootTable = toml::find<toml::table>(bpCopy, rootTable);

    // For each entry in the "TASKS" table, parse recursively
    for (const auto &kv : tasksRootTable) {
      std::string topName = kv.first;
      // Example: "transform" or "properties"
      // Convert to uppercase for consistency
      std::transform(topName.begin(), topName.end(), topName.begin(), ::toupper);

      // Recurse to parse subcommands. The parseTaskRec function will:
      //  - parse the current table if it has mandatory fields
      //  - parse deeper subcommands if present
      if (kv.second.is_table()) {
        parseTaskRec(topName, kv.second, tasksMap);
      }
    }
  }
  catch (const std::exception &ex) {
    throw std::runtime_error("Error building blueprint tasks map: " + std::string(ex.what()));
  }

  return tasksMap;
}

} // end anonymous namespace

///////////////////////////////////////////////////////////////////////////////
/// Retrieves a copy of the statically cached blueprint map.
/// @return A copy of the unordered_map mapping blueprint keys to BlueprintKey
/// objects.
/// @throws std::runtime_error If blueprint parsing or inheritance expansion
/// fails.
///////////////////////////////////////////////////////////////////////////////
std::unordered_map<std::string, BlueprintKey> BlueprintParser::getKeysMap() {
  // Return a copy of the static blueprint map.
  return getKeysMapRef();
}

std::unordered_map<std::string, BlueprintKey>
BlueprintParser::getInternalKeysMap() {
  // Return a copy of the static blueprint map.
  return getInternalKeysMapRef();
}

std::unordered_map<std::string, BlueprintTask> BlueprintParser::getTasksMap() {
  // Return a copy of the static blueprint map.
  return getTasksMapRef();
}

BlueprintTadah BlueprintParser::getBlueprintTadah() {
  // Return a copy of the static blueprint map.
  return getBlueprintTadahRef();
}
/**
 * @brief Returns a reference to a statically cached BlueprintTadah object.
 *
 * This version includes corrected handling for:
 *  - Core TADAH fields (logo, version, website, description, longDescription, examples).
 *  - Optional "keys" subtable, each entry parsed as a BlueprintKey and added to blueprintKeys.
 *
 * @return Const reference to the fully loaded and expanded BlueprintTadah.
 * @throws std::runtime_error if parsing or inheritance expansion fails, or if
 *         mandatory fields are missing in the [TADAH] table.
 */
const BlueprintTadah &BlueprintParser::getBlueprintTadahRef() {
  static BlueprintTadah blueprintTadah = []() -> BlueprintTadah {
    BlueprintTadah bt;

    // ------------------------------------------------------------
    // 1) Parse the blueprint file from CMRC
    // ------------------------------------------------------------
    auto fs = cmrc::CORE::get_filesystem();
    auto toml_file = fs.open(blueprintFilePath);
    std::stringstream ss;
    ss << toml_file.begin();
    toml::value blueprint = toml::parse(ss);

    // ------------------------------------------------------------
    // 2) Prepare single-level "keys" inheritance if required
    // ------------------------------------------------------------
    prepareKeysInheritanceOneLevelWithMerge(blueprint);

    // ------------------------------------------------------------
    // 3) Expand all inheritance references recursively
    // ------------------------------------------------------------
    toml::value bpCopy = blueprint;
    expandInheritance(bpCopy, blueprint, "");

    // ------------------------------------------------------------
    // 4) Access the [TADAH] subtable
    // ------------------------------------------------------------
    const toml::value &tadahValue = toml::find(bpCopy, "TADAH");
    if (!tadahValue.is_table()) {
      throw std::runtime_error("Section [TADAH] must be a table in "+blueprintFilePath);
    }
    const toml::table &tadahTable = tadahValue.as_table();

    // ------------------------------------------------------------
    // 5) Retrieve mandatory fields
    // ------------------------------------------------------------
    // logo
    {
      toml::table::const_iterator it = tadahTable.find("logo");
      if (it == tadahTable.end()) {
        throw std::runtime_error("[TADAH] is missing mandatory field 'logo'");
      }
      bt.logo = toml::get<std::string>(it->second);
    }
    // version
    {
      toml::table::const_iterator it = tadahTable.find("version");
      if (it == tadahTable.end()) {
        throw std::runtime_error("[TADAH] is missing mandatory field 'version'");
      }
      bt.version = toml::get<std::string>(it->second);
    }
    // website
    {
      toml::table::const_iterator it = tadahTable.find("website");
      if (it == tadahTable.end()) {
        throw std::runtime_error("[TADAH] is missing mandatory field 'website'");
      }
      bt.website = toml::get<std::string>(it->second);
    }
    // description
    {
      toml::table::const_iterator it = tadahTable.find("description");
      if (it == tadahTable.end()) {
        throw std::runtime_error("[TADAH] is missing mandatory field 'description'");
      }
      bt.description = toml::get<std::string>(it->second);
    }
    // longDescription
    {
      toml::table::const_iterator it = tadahTable.find("long_description");
      if (it == tadahTable.end()) {
        throw std::runtime_error("[TADAH] is missing mandatory field 'long_description'");
      }
      bt.longDescription = toml::get<std::string>(it->second);
    }
    // examples (array of strings)
    {
      toml::table::const_iterator it = tadahTable.find("examples");
      if (it == tadahTable.end()) {
        throw std::runtime_error("[TADAH] is missing mandatory field 'examples'");
      }
      // throws if not an array of strings
      bt.examples = toml::get<std::vector<std::string>>(it->second);
    }

    // ------------------------------------------------------------
    // 6) Parse optional "keys" subtable into a vector of BlueprintKey
    // ------------------------------------------------------------
    {
      toml::table::const_iterator it = tadahTable.find("keys");
      if (it != tadahTable.end()) {
        // Ensure "keys" is a table
        if (!it->second.is_table()) {
          throw std::runtime_error("[TADAH].keys must be a table if present");
        }
        const toml::table &keysTbl = it->second.as_table();

        // For each entry under [TADAH.keys], parse as a BlueprintKey
        // The name is "TADAH.<entryName>"
        for (toml::table::const_iterator kv = keysTbl.begin(); kv != keysTbl.end(); ++kv) {
          std::string dottedKeyName = /*std::string("TADAH.") +*/ kv->first;
          BlueprintKey newKey = buildBlueprintKey(dottedKeyName, kv->second);
          bt.blueprintKeys.push_back(newKey);
        }
      }
    }

    // ------------------------------------------------------------
    // Return the fully-constructed structure
    // ------------------------------------------------------------
    return bt;
  }();

  return blueprintTadah;
}
///////////////////////////////////////////////////////////////////////////////
/// Retrieves a const reference to the statically cached blueprint map.
/// Uses a static local variable to guarantee one-time initialization.
/// @return Const reference to the unordered_map mapping blueprint keys to
/// BlueprintKey objects.
/// @throws std::runtime_error If blueprint parsing or inheritance expansion
/// fails.
///////////////////////////////////////////////////////////////////////////////
const std::unordered_map<std::string, BlueprintKey> &
BlueprintParser::getKeysMapRef() {
  static std::unordered_map<std::string, BlueprintKey> blueprintAllowedMap =
      buildBlueprintMap("ALLOWED_KEYS");
  return blueprintAllowedMap;
}
const std::unordered_map<std::string, BlueprintKey> &
BlueprintParser::getInternalKeysMapRef() {
  static std::unordered_map<std::string, BlueprintKey> blueprintInternalMap =
      buildBlueprintMap("INTERNAL_KEYS");
  return blueprintInternalMap;
}
const std::unordered_map<std::string, BlueprintTask> &
BlueprintParser::getTasksMapRef() {
  static std::unordered_map<std::string, BlueprintTask> blueprintTasksMap =
      buildBlueprintTasksMap("TASKS");
  return blueprintTasksMap;
}

std::string BlueprintParser::findClosestKey(
    const std::string &key,
    const std::unordered_map<std::string, BlueprintKey> &map) {
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
