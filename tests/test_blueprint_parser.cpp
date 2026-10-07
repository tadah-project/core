#include "catch2/catch.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include "tadah/core/blueprint_parser.h"
#include "tadah/core/blueprint_validator.h"
#include <cctype>
#include <sstream>

using namespace tadah::core;

TEST_CASE("BlueprintParser loads ALLOWED_KEYS from blueprint_tadah.toml", "[BlueprintParser]")
{
  // Retrieve the statically cached blueprint map.
  const auto &blueprintMap = BlueprintParser::getKeysMapRef();

  SECTION("The blueprint map is not empty")
  {
    // The blueprint map must contain entries.
    REQUIRE(!blueprintMap.empty());
  }

  SECTION("The CONFIG key is parsed correctly")
  {
    // The CONFIG key should be present in uppercase.
    auto it = blueprintMap.find("CONFIG");
    REQUIRE(it != blueprintMap.end());
    const BlueprintKey &configKey = it->second;

    // Validate expected fields.
    REQUIRE(configKey.value_type == std::vector<std::string>{"STRING"});
    REQUIRE(configKey.value_format == std::vector<std::string>{"<file>"});
    REQUIRE(configKey.value_N == std::vector<int>{1});
    REQUIRE(configKey.description == "Path to a configuration file.");
    REQUIRE(configKey.examples.size() >= 1);
    REQUIRE(configKey.examples[0] == "config.tadah");

    // Validate CLI-specific fields.
    REQUIRE(configKey.cli_long_flag == "--config");
    REQUIRE(configKey.cli_short_flag == "-c");
  }

  SECTION("The EMBF0 key: STRING, default ZERO, no CLI flag")
  {
    auto it = blueprintMap.find("EMBF0");
    REQUIRE(it != blueprintMap.end());
    const BlueprintKey &k = it->second;
    REQUIRE(k.value_type == std::vector<std::string>{"STRING"});
    REQUIRE(k.value_N == std::vector<int>{1});
    REQUIRE(k.default_value == std::vector<std::string>{"ZERO"});
    REQUIRE(k.examples == std::vector<std::string>{"ZERO", "FREE"});
    REQUIRE(k.cli_long_flag.empty());
    REQUIRE(k.cli_short_flag.empty());
  }

  SECTION("The VERBOSE key is parsed correctly")
  {
    // The VERBOSE key should be present (converted to uppercase).
    auto it = blueprintMap.find("VERBOSE");
    REQUIRE(it != blueprintMap.end());
    const BlueprintKey &verboseKey = it->second;

    // Validate expected fields.
    REQUIRE(verboseKey.value_type == std::vector<std::string>{"UINT"});
    REQUIRE(verboseKey.value_format == std::vector<std::string>{"<unsigned integer>"});
    REQUIRE(verboseKey.value_N == std::vector<int>{1});
    REQUIRE(verboseKey.description ==
            "Verbosity level. 0-2: ERROR, WARNING, INFO.");
    REQUIRE(verboseKey.examples == std::vector<std::string>{"2"});

    // Validate default value and CLI options.
    REQUIRE(verboseKey.default_value[0] == "1");
    REQUIRE(verboseKey.cli_long_flag == "--verbose");
    REQUIRE(verboseKey.cli_short_flag == "-v");
  }

  SECTION("The BIAS key is parsed correctly")
  {
    // The BIAS key should be present.
    auto it = blueprintMap.find("BIAS");
    REQUIRE(it != blueprintMap.end());
    const BlueprintKey &biasKey = it->second;

    // Validate expected fields.
    REQUIRE(biasKey.value_type == std::vector<std::string>{"BOOL"});
    REQUIRE(biasKey.value_format == std::vector<std::string>{"<boolean>"});
    REQUIRE(biasKey.value_N == std::vector<int>{1});
    REQUIRE(biasKey.description.find("Per-species intercept") != std::string::npos);
    REQUIRE(biasKey.examples == std::vector<std::string>{"true", "false"});

    // Validate that the default value (if defined) is as expected.
    REQUIRE(biasKey.default_value[0] == "false");
  }

  SECTION("The OALGO key is parsed correctly")
  {
    // The OALGO key should be present.
    auto it = blueprintMap.find("OALGO");
    REQUIRE(it != blueprintMap.end());
    const BlueprintKey &oalgoKey = it->second;
    // Validate the value type list.
    std::vector<std::string> expectedValueType = {"INT", "INT DOUBLE"};
    REQUIRE(oalgoKey.value_type == expectedValueType);

    // Validate the expected format list and constraints.
    REQUIRE(oalgoKey.value_format == std::vector<std::string>{"<option>", "<option> <value>"});
    REQUIRE(oalgoKey.value_N == std::vector<int>{-2});

    // Verify that the description contains key text.
    REQUIRE(oalgoKey.description.find("This key controls the optimization algorithm") != std::string::npos);

    // Verify that example usage strings are present.
    REQUIRE(oalgoKey.examples.size() >= 5);
  }

  SECTION("The NUMERIC key is parsed correctly")
  {
    // The NUMERIC key should be present.
    auto it = blueprintMap.find("NUMERIC");
    REQUIRE(it != blueprintMap.end());
    const BlueprintKey &numericKey = it->second;

    // Validate expected fields.
    REQUIRE(numericKey.value_type == std::vector<std::string>{"UINT"});
    REQUIRE(numericKey.value_format == std::vector<std::string>{"<unsigned integer>"});
    REQUIRE(numericKey.value_N == std::vector<int>{1});
    REQUIRE(numericKey.description ==
            "Numeric output precision.");
    REQUIRE(numericKey.examples == std::vector<std::string>{"18"});
    
    // Validate CLI-specific fields.
    REQUIRE(numericKey.cli_long_flag == "--numeric");
    REQUIRE(numericKey.default_value[0] == "18");
    // A default-inserted NUMERIC must never lower potential-file precision
    // below round-trip (resolve_pot_digits treats any stored value as
    // explicit, so the default has to be lossless on its own).
    REQUIRE(std::stoi(numericKey.default_value[0]) >= 17);
  }

  SECTION("The blueprint map contains only ALLOWED_KEYS entries")
  {
    // Keys not defined under ALLOWED_KEYS (e.g. TASKS) should not be present.
    // However, only ALLOWED_KEYS are processed.
    // Similarly, the GLOBALS section should not appear.
    REQUIRE(blueprintMap.find("GLOBALS") == blueprintMap.end());
  }
}

/**
 * @brief Tests that an empty map returns an empty string when searching for a closest key.
 */
TEST_CASE("BlueprintParser::findClosestKey with empty map", "[findClosestKey]")
{
  std::unordered_map<std::string, BlueprintKey> emptyMap;
  std::string result = BlueprintParser::findClosestKey("ANYKEY", emptyMap);

  REQUIRE(result.empty());
}

/**
 * @brief Tests that an exact match returns the exact key if the key exists in the map.
 */
TEST_CASE("BlueprintParser::findClosestKey with exact match", "[findClosestKey]")
{
  BlueprintKey dummyKey;
  std::unordered_map<std::string, BlueprintKey> map{
    {"ALLOWED_KEYS.CONFIG", dummyKey},
    {"INTERNAL_KEYS.DEBUG", dummyKey}
  };

  std::string result = BlueprintParser::findClosestKey("ALLOWED_KEYS.CONFIG", map);
  REQUIRE(result == "ALLOWED_KEYS.CONFIG");
}

/**
 * @brief Tests that the method chooses the key with the smallest edit distance when multiple keys exist.
 *
 * Uses a situation where "ALOWED_KEYS.CONFIG" is closer to "ALLOWED_KEYS.CONFIG" than to "ALLOWED_KEYS.CONFING".
 */
TEST_CASE("BlueprintParser::findClosestKey chooses the minimum edit distance", "[findClosestKey]")
{
  BlueprintKey dummyKey;
  std::unordered_map<std::string, BlueprintKey> map{
    {"ALLOWED_KEYS.CONFIG", dummyKey},
    {"ALLOWED_KEYS.CONFING", dummyKey},
    {"SOME_OTHER_KEY", dummyKey}
  };

  std::string result = BlueprintParser::findClosestKey("ALOWED_KEYS.CONFIG", map);
  REQUIRE(result == "ALLOWED_KEYS.CONFIG");
}

/**
 * @brief Tests behavior when the user input is very different from the available keys.
 *
 * Ensures that some non-empty string is still returned as the best match when the map is not empty.
 */
TEST_CASE("BlueprintParser::findClosestKey with large distance key", "[findClosestKey]")
{
  BlueprintKey dummyKey;
  std::unordered_map<std::string, BlueprintKey> map{
    {"ALLOWED_KEYS.CONFIG", dummyKey},
    {"INTERNAL_KEYS.DEBUG", dummyKey}
  };

  std::string result = BlueprintParser::findClosestKey("COMPLETELY_UNRELATED_KEY", map);

  // A non-empty result is expected since at least one key is in the map.
  REQUIRE_FALSE(result.empty());
}


/* Pins: every example of every key of the blueprint is either a value the
 * key's format accepts (BlueprintValidator on its value_type, as the
 * configuration parser checks a value) or a configuration line that starts
 * with the key itself or with its command-line flag, possibly followed by
 * more lines (the TYPE2B D2_mJoin block). CoreUserConfigParser prints each
 * example as "<KEY>   <example>" under "Examples:" when it refuses a value,
 * so an example that is neither shows a line that is not one of the key's.
 * Before the fix: two examples were neither. SETFL's second was a whole
 * configuration starting with another key, "INIT2B    true" (the parser
 * printed "SETFL   INIT2B    true"), and SIGMA's was quoted in backticks,
 * "`2 1.2 2.2 2.3 3.3`", which its format (INT {DOUBLE}) does not accept. */
TEST_CASE("Every blueprint example is a value of its key or a line that "
          "starts with the key",
          "[BlueprintParser]") {
  const auto upper = [](std::string t) {
    for (char &ch : t) ch = static_cast<char>(std::toupper(
                           static_cast<unsigned char>(ch)));
    return t;
  };
  std::size_t checked = 0;
  for (const auto &kv : BlueprintParser::getKeysMapRef()) {
    const BlueprintKey &k = kv.second;
    if (k.examples.empty()) continue;
    const BlueprintValidator validator(k.value_type);
    for (const std::string &example : k.examples) {
      ++checked;
      std::istringstream words(example);
      std::string first;
      words >> first;
      const bool line_of_key =
          upper(first) == kv.first ||
          (!k.cli_long_flag.empty() && first == k.cli_long_flag) ||
          (!k.cli_short_flag.empty() && first == k.cli_short_flag);
      INFO("key " << kv.first << ", example:\n" << example);
      CHECK((line_of_key || validator.validate(example)));
    }
  }
  CHECK(checked > 200);
}
