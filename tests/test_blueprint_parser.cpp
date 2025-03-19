#include "catch2/catch.hpp"
#include <string>
#include <vector>
#include <iostream>
#include "tadah/core/blueprint_parser.h"

using namespace tadah::core;

TEST_CASE("BlueprintParser loads ALLOWED_KEYS from config_blueprint.toml", "[BlueprintParser]")
{
  // Retrieve the statically cached blueprint map.
  const auto &blueprintMap = BlueprintParser::getMapRef();

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
    REQUIRE(configItem.value_type == std::vector<std::string>{"string"});
    REQUIRE(configItem.value_format == std::vector<std::string>{"<file>"});
    REQUIRE(configItem.value_N == std::vector<int>{1});
    REQUIRE(configItem.description == "Path to configuration file.");
    REQUIRE(configItem.examples.size() >= 1);
    REQUIRE(configItem.examples[0] == "config.tadah");

    // Validate CLI-specific fields.
    REQUIRE(configItem.long_option == "--config");
    REQUIRE(configItem.short_option == "-c");
  }

  SECTION("The VERBOSE key is parsed correctly")
  {
    // The VERBOSE key should be present (converted to uppercase).
    auto it = blueprintMap.find("VERBOSE");
    REQUIRE(it != blueprintMap.end());
    const BlueprintItem &verboseItem = it->second;

    // Validate expected fields.
    REQUIRE(verboseItem.value_type == std::vector<std::string>{"int"});
    REQUIRE(verboseItem.value_format == std::vector<std::string>{"<N>"});
    REQUIRE(verboseItem.value_N == std::vector<int>{1});
    REQUIRE(verboseItem.description ==
            "Verbosity level. 0-4: ERROR, WARNING, INFO, DEBUG, TRACE.");
    REQUIRE(verboseItem.examples == std::vector<std::string>{"1", "2", "0"});

    // Validate default value and CLI options.
    REQUIRE(verboseItem.defaultValue == "1");
    REQUIRE(verboseItem.long_option == "--verbose");
    REQUIRE(verboseItem.short_option == "-V");
  }

  SECTION("The BIAS key is parsed correctly")
  {
    // The BIAS key should be present.
    auto it = blueprintMap.find("BIAS");
    REQUIRE(it != blueprintMap.end());
    const BlueprintItem &biasItem = it->second;

    // Validate expected fields.
    REQUIRE(biasItem.value_type == std::vector<std::string>{"bool"});
    REQUIRE(biasItem.value_format == std::vector<std::string>{"true | false"});
    REQUIRE(biasItem.value_N == std::vector<int>{1});
    REQUIRE(biasItem.description == "Bias");
    REQUIRE(biasItem.examples == std::vector<std::string>{"true"});

    // Validate that the default value (if defined) is as expected.
    REQUIRE(biasItem.defaultValue == "false");
  }

  SECTION("The OALGO key is parsed correctly")
  {
    // The OALGO key should be present.
    auto it = blueprintMap.find("OALGO");
    REQUIRE(it != blueprintMap.end());
    const BlueprintItem &oalgoItem = it->second;
    std::cout << "oalgoItem.description: " << oalgoItem.description << std::endl;

    std::cout << "oalgoItem.description: " << oalgoItem.description << std::endl;
    // Validate the value type list.
    std::vector<std::string> expectedValueType = {"<int>", "<int> <double>"};
    REQUIRE(oalgoItem.value_type == expectedValueType);
    std::cout << "oalgoItem.description: " << oalgoItem.description << std::endl;

    // Validate the expected format list and constraints.
    REQUIRE(oalgoItem.value_format == std::vector<std::string>{"<option>", "<option> <value>"});
    std::cout << "oalgoItem.description: " << oalgoItem.description << std::endl;
    REQUIRE(oalgoItem.value_N == std::vector<int>{-2});
    std::cout << "oalgoItem.description: " << oalgoItem.description << std::endl;

    // Verify that the description contains key text.
    REQUIRE(oalgoItem.description.find("This key controls the optimization algorithm") != std::string::npos);
    std::cout << "oalgoItem.description: " << oalgoItem.description << std::endl;

    // Verify that example usage strings are present.
    REQUIRE(oalgoItem.examples.size() >= 5);
    std::cout << "oalgoItem.description: " << oalgoItem.description << std::endl;
  }

  SECTION("The NUMERIC key is parsed correctly")
  {
    // The NUMERIC key should be present.
    auto it = blueprintMap.find("NUMERIC");
    REQUIRE(it != blueprintMap.end());
    const BlueprintItem &numericItem = it->second;

    // Validate expected fields.
    REQUIRE(numericItem.value_type == std::vector<std::string>{"int"});
    REQUIRE(numericItem.value_format == std::vector<std::string>{"<N>"});
    REQUIRE(numericItem.value_N == std::vector<int>{1});
    REQUIRE(numericItem.description ==
            "Numeric output precision. Either number of decimal places or scientific notation. Depending on context.");
    REQUIRE(numericItem.examples == std::vector<std::string>{"12"});
    
    // Validate CLI-specific fields.
    REQUIRE(numericItem.long_option == "--numeric");
    REQUIRE(numericItem.defaultValue == "12");
  }

  SECTION("The blueprint map contains only ALLOWED_KEYS entries")
  {
    // Keys not defined under ALLOWED_KEYS (e.g. TASKS) should not be present.
    // However, only ALLOWED_KEYS are processed.
    // Similarly, the GLOBALS section should not appear.
    REQUIRE(blueprintMap.find("GLOBALS") == blueprintMap.end());
  }
}
