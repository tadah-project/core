#include "catch2/catch.hpp"
#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>
#include "tadah/core/blueprint_parser.h"

using namespace tadah::core;

TEST_CASE("BlueprintParser loads ALLOWED_KEYS from config_blueprint.toml", "[BlueprintParser]")
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
    const BlueprintItem &configItem = it->second;

    // Validate expected fields.
    REQUIRE(configItem.value_type == std::vector<std::string>{"STRING"});
    REQUIRE(configItem.value_format == std::vector<std::string>{"<file>"});
    REQUIRE(configItem.value_N == std::vector<int>{1});
    REQUIRE(configItem.description == "Path to configuration file.");
    REQUIRE(configItem.examples.size() >= 1);
    REQUIRE(configItem.examples[0] == "config.tadah");

    // Validate CLI-specific fields.
    REQUIRE(configItem.cli_long_flag == "--config");
    REQUIRE(configItem.cli_short_flag == "-c");
  }

  SECTION("The VERBOSE key is parsed correctly")
  {
    // The VERBOSE key should be present (converted to uppercase).
    auto it = blueprintMap.find("VERBOSE");
    REQUIRE(it != blueprintMap.end());
    const BlueprintItem &verboseItem = it->second;

    // Validate expected fields.
    REQUIRE(verboseItem.value_type == std::vector<std::string>{"UINT"});
    REQUIRE(verboseItem.value_format == std::vector<std::string>{"<unsigned integer>"});
    REQUIRE(verboseItem.value_N == std::vector<int>{1});
    REQUIRE(verboseItem.description ==
            "Verbosity level. 0-4: ERROR, WARNING, INFO, DEBUG, TRACE.");
    REQUIRE(verboseItem.examples == std::vector<std::string>{"1", "2", "3"});

    // Validate default value and CLI options.
    REQUIRE(verboseItem.defaultValue[0] == "1");
    REQUIRE(verboseItem.cli_long_flag == "--verbose");
    REQUIRE(verboseItem.cli_short_flag == "-V");
  }

  SECTION("The BIAS key is parsed correctly")
  {
    // The BIAS key should be present.
    auto it = blueprintMap.find("BIAS");
    REQUIRE(it != blueprintMap.end());
    const BlueprintItem &biasItem = it->second;

    // Validate expected fields.
    REQUIRE(biasItem.value_type == std::vector<std::string>{"BOOL"});
    REQUIRE(biasItem.value_format == std::vector<std::string>{"<boolean>"});
    REQUIRE(biasItem.value_N == std::vector<int>{1});
    REQUIRE(biasItem.description == "Bias");
    REQUIRE(biasItem.examples == std::vector<std::string>{"true", "false"});

    // Validate that the default value (if defined) is as expected.
    REQUIRE(biasItem.defaultValue[0] == "false");
  }

  SECTION("The OALGO key is parsed correctly")
  {
    // The OALGO key should be present.
    auto it = blueprintMap.find("OALGO");
    REQUIRE(it != blueprintMap.end());
    const BlueprintItem &oalgoItem = it->second;
    // Validate the value type list.
    std::vector<std::string> expectedValueType = {"INT", "INT DOUBLE"};
    REQUIRE(oalgoItem.value_type == expectedValueType);

    // Validate the expected format list and constraints.
    REQUIRE(oalgoItem.value_format == std::vector<std::string>{"<option>", "<option> <value>"});
    REQUIRE(oalgoItem.value_N == std::vector<int>{-2});

    // Verify that the description contains key text.
    REQUIRE(oalgoItem.description.find("This key controls the optimization algorithm") != std::string::npos);

    // Verify that example usage strings are present.
    REQUIRE(oalgoItem.examples.size() >= 5);
  }

  SECTION("The NUMERIC key is parsed correctly")
  {
    // The NUMERIC key should be present.
    auto it = blueprintMap.find("NUMERIC");
    REQUIRE(it != blueprintMap.end());
    const BlueprintItem &numericItem = it->second;

    // Validate expected fields.
    REQUIRE(numericItem.value_type == std::vector<std::string>{"UINT"});
    REQUIRE(numericItem.value_format == std::vector<std::string>{"<unsigned integer>"});
    REQUIRE(numericItem.value_N == std::vector<int>{1});
    REQUIRE(numericItem.description ==
            "Numeric output precision. Either number of decimal places or scientific notation. Depending on context.");
    REQUIRE(numericItem.examples == std::vector<std::string>{"12"});
    
    // Validate CLI-specific fields.
    REQUIRE(numericItem.cli_long_flag == "--numeric");
    REQUIRE(numericItem.defaultValue[0] == "12");
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
  std::unordered_map<std::string, BlueprintItem> emptyMap;
  std::string result = BlueprintParser::findClosestKey("ANYKEY", emptyMap);

  REQUIRE(result.empty());
}

/**
 * @brief Tests that an exact match returns the exact key if the key exists in the map.
 */
TEST_CASE("BlueprintParser::findClosestKey with exact match", "[findClosestKey]")
{
  BlueprintItem dummyItem;
  std::unordered_map<std::string, BlueprintItem> map{
    {"ALLOWED_KEYS.CONFIG", dummyItem},
    {"INTERNAL_KEYS.DEBUG", dummyItem}
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
  BlueprintItem dummyItem;
  std::unordered_map<std::string, BlueprintItem> map{
    {"ALLOWED_KEYS.CONFIG", dummyItem},
    {"ALLOWED_KEYS.CONFING", dummyItem},
    {"SOME_OTHER_KEY", dummyItem}
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
  BlueprintItem dummyItem;
  std::unordered_map<std::string, BlueprintItem> map{
    {"ALLOWED_KEYS.CONFIG", dummyItem},
    {"INTERNAL_KEYS.DEBUG", dummyItem}
  };

  std::string result = BlueprintParser::findClosestKey("COMPLETELY_UNRELATED_KEY", map);

  // A non-empty result is expected since at least one key is in the map.
  REQUIRE_FALSE(result.empty());
}

