#include "catch2/catch.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include "tadah/core/blueprint_parser.h"

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

