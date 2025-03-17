#include <tadah/core/blueprint_parser.h>
#include <iostream>

namespace tadah {
namespace core {

BlueprintParser::BlueprintParser(const std::string &blueprintFile)
  : blueprintFile_(blueprintFile)
{
  try {
    blueprint_ = toml::parse(blueprintFile_);
  }
  catch (const std::exception &ex) {
    throw std::runtime_error("Error parsing blueprint file: " + std::string(ex.what()));
  }
}

std::unordered_map<std::string, BlueprintItem> BlueprintParser::parse() {
  // Work on a copy to support in-place inheritance expansion.
  toml::value bpCopy = blueprint_;
  expandInheritance(bpCopy);

  std::unordered_map<std::string, BlueprintItem> blueprintMap;
  if (bpCopy.contains("ALLOWED_KEYS")) {
    const auto &allowed = toml::find<toml::table>(bpCopy, "ALLOWED_KEYS");
    for (const auto &item : allowed) {
      std::string fullKey = "ALLOWED_KEYS." + item.first;
      blueprintMap[fullKey] = buildBlueprintItem(fullKey, item.second);
    }
  }
  return blueprintMap;
}

void BlueprintParser::expandInheritance(toml::value &table, const std::string &parentScope) {
  if (table.is_table()) {
    // Process each sub-key recursively
    for (auto &kv : table.as_table()) {
      if (kv.second.is_table()) {
        std::string newScope = parentScope.empty() ? kv.first : (parentScope + "." + kv.first);
        expandInheritance(kv.second, newScope);
      }
    }
    // Process the INHERIT key if present.
    if (table.contains("INHERIT")) {
      try {
        auto inheritList = toml::find<std::vector<std::string>>(table, "INHERIT");
        for (const std::string &inheritPath : inheritList) {
          const toml::value &baseTable = getTableByPath(blueprint_, inheritPath);
          mergeTables(table, baseTable);
        }
      }
      catch (const std::exception &ex) {
        throw std::runtime_error("Error during inheritance expansion: " + std::string(ex.what()));
      }
      table.as_table().erase("INHERIT");
    }
  }
}

const toml::value& BlueprintParser::getTableByPath(const toml::value &root, const std::string &path) const {
  std::vector<std::string> parts;
  boost::split(parts, path, boost::is_any_of("."));
  const toml::value *curr = &root;
  for(const std::string &part : parts) {
    if (!curr->is_table())
      throw std::runtime_error("Invalid path in inheritance: " + path);
    // Use generic find() so that returned type is const toml::value&
    curr = &toml::find(*curr, part);
  }
  return *curr;
}

void BlueprintParser::mergeTables(toml::value &dest, const toml::value &source) {
  const auto &srcTable = source.as_table();
  auto &destTable = dest.as_table();
  for (const auto &kv : srcTable) {
    // Only copy if the destination does not have the key.
    if (destTable.find(kv.first) == destTable.end()) {
      destTable[kv.first] = kv.second;
    }
  }
}

BlueprintItem BlueprintParser::buildBlueprintItem(const std::string &fullKey, const toml::value &table) {
  BlueprintItem item;
  item.keyName = fullKey;
  try {
    item.value_type = toml::find<std::vector<std::string>>(table, "value_type");
    item.value_format = toml::find<std::vector<std::string>>(table, "value_format");
    item.value_N = toml::find<std::vector<int>>(table, "value_N");
    item.description = toml::find<std::string>(table, "description");
    item.examples = toml::find<std::vector<std::string>>(table, "examples");
  }
  catch (const std::exception &ex) {
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

} // namespace core
} // namespace tadah

