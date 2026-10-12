#include "catch2/catch.hpp"
#include <vector>
#include <string>
#include <iostream> // For debug if needed

// Assume "blueprint_validator.h" provides tadah::core::BlueprintValidator,
// or adapt the path as necessary.
#include <tadah/core/blueprint_validator.h>

/**
 * @brief Helper that returns true if all provided examples pass validation
 *        for a given blueprint. This approach is used for brevity.
 */
static bool allExamplesValidate(const std::string &blueprint,
                                const std::vector<std::string> &examples) {
  tadah::core::BlueprintValidator validator(blueprint);
  for (auto &example : examples) {
    if (!validator.validate(example)) {
      return false;
    }
  }
  return true;
}

/**
 * @brief Helper that returns true if any of the provided strings pass validation.
 *        This checks negative/edge cases that are expected to fail.
 */
static bool anyStringValidates(const std::string &blueprint,
                               const std::vector<std::string> &negatives) {
  tadah::core::BlueprintValidator validator(blueprint);
  for (auto &neg : negatives) {
    if (validator.validate(neg)) {
      return true;
    }
  }
  return false;
}

TEST_CASE("TYPES.BOOL: value_type = [\"BOOL\"]", "[blueprint]") {
  // Single bool: "true" or "false"
  std::string blueprint = "BOOL";
  std::vector<std::string> validCases = {"true", "false"};
  std::vector<std::string> invalidCases = {"True", "False", "TRUE", "maybe", ""};

  SECTION("Valid examples must pass") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Invalid examples must fail") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

TEST_CASE("TYPES.BOOLS: value_type = [\"{BOOL}\"]", "[blueprint]") {
  // Repeated bool tokens
  std::string blueprint = "{BOOL}";
  // Some valid sequences: "true", "false", "true true false"
  std::vector<std::string> validCases = {"true", "false", "true true false", "false true"};
  // Some invalid sequences
  std::vector<std::string> invalidCases = {"true false maybe", "maybe", "TRUE"};

  SECTION("Valid examples must pass") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Invalid examples must fail") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

TEST_CASE("TYPES.INT: value_type = [\"INT\"]", "[blueprint]") {
  std::string blueprint = "INT";
  std::vector<std::string> validCases = {"20", "-10", "0", "+5"};
  std::vector<std::string> invalidCases = {"1.0", "abc", "--10","0 +42","20.5", "abc", "2 x 3", ""};

  SECTION("Valid repeated integer examples must pass") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Invalid sequences must fail") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

TEST_CASE("TYPES.UINT: value_type = [\"UINT\"]", "[blueprint]") {
  std::string blueprint = "UINT";
  // "UINT" is 0 or positive integer
  std::vector<std::string> validCases = {"0", "20", "9999"};
  std::vector<std::string> invalidCases = {"-1", "+1", "abc", "", "03"};

  SECTION("Valid unsigned examples must pass") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Invalid unsigned examples must fail") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

TEST_CASE("TYPES.UINTS: value_type = [\"{UINT}\"]", "[blueprint]") {
  std::string blueprint = "{UINT}";
  std::vector<std::string> validCases = {"0", "20 5 3", "10", "1 2 3 9999"};
  std::vector<std::string> invalidCases = {"-1", "abc", "5 -1"};

  SECTION("Valid repeated uint examples must pass") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Invalid repeated uint examples must fail") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

TEST_CASE("TYPES.DOUBLE: value_type = [\"DOUBLE\"]", "[blueprint]") {
  std::string blueprint = "DOUBLE";
  // e.g., "2.0", "-1.0", "3.5e+2"
  std::vector<std::string> validCases = {"2.0", "-1.0", "3.14", "3.5e+2", "0", "-10"};
  std::vector<std::string> invalidCases = {"abc", ".", "--3.0", ""};

  SECTION("Valid floating examples must pass") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Invalid floating examples must fail") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

TEST_CASE("TYPES.DOUBLES: value_type = [\"{DOUBLE}\"]", "[blueprint]") {
  std::string blueprint = "{DOUBLE}";
  // Repeated floating
  std::vector<std::string> validCases = {"2.0 -4.65 0.4", "-1.0", "1.0 2.0 3.0 4.0"};
  std::vector<std::string> invalidCases = {"abc", "1. 2. 3..", ""};

  SECTION("Valid repeated float examples must pass") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Invalid repeated float examples must fail") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

TEST_CASE("TYPES.STRING: value_type = [\"STRING\"]", "[blueprint]") {
  std::string blueprint = "STRING";
  // Single string with at least one letter
  std::vector<std::string> validCases = {"validString", "/path/to/file", "H", "*hello", "XYZ123"};
  std::vector<std::string> invalidCases = {"", "1234", "   ", "/"};

  SECTION("Valid single string examples must pass") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Invalid single string examples must fail") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

TEST_CASE("TYPES.STRINGS: value_type = [\"{STRING}\"]", "[blueprint]") {
  std::string blueprint = "{STRING}";
  // Repeated sets of valid strings
  std::vector<std::string> validCases = {
    "string1 string2",
    "path1 /path/to/file2",
    "alpha12 Beta13 gamma14",
    "a b c"
  };
  std::vector<std::string> invalidCases = {"", "123 456", "   "};

  SECTION("Valid repeated string examples must pass") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Invalid repeated string examples must fail") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

TEST_CASE("TYPES.STRINGS handles long path lists", "[blueprint][long]") {
  tadah::core::BlueprintValidator validator("{STRING}");

  auto join = [](const std::vector<std::string> &tokens) {
    std::string out;
    for (std::size_t i = 0; i < tokens.size(); ++i) {
      if (i > 0) {
        out += ' ';
      }
      out += tokens[i];
    }
    return out;
  };

  std::vector<std::string> validTokens;
  validTokens.reserve(90);
  for (int i = 0; i < 90; ++i) {
    validTokens.push_back("/tmp/HS_MD_GAMMA_ONLY_" + std::to_string(i) + "/OUTCAR");
  }

  const std::string validLongLine = join(validTokens);
  REQUIRE(validLongLine.size() > 2048);
  REQUIRE(validator.validate(validLongLine));

  validTokens[45] = "123456";
  const std::string invalidLongLine = join(validTokens);
  REQUIRE(invalidLongLine.size() > 2048);
  REQUIRE_FALSE(validator.validate(invalidLongLine));
}

/**
 * @brief Test the more complicated pattern:
 *        "STRING {[INT | DOUBLE]} {STRING STRING}"
 */
TEST_CASE("TYPE2B: value_type = [\"STRING {[INT | DOUBLE]} {STRING STRING}\"]", "[blueprint]") {
  std::string blueprint = "STRING {[INT | DOUBLE]} {STRING STRING}";
  tadah::core::BlueprintValidator validator(blueprint);

  SECTION("Valid: minimal form with INT variant") {
    REQUIRE(validator.validate("D2_LJ 11 Ti Ti"));
  }
  SECTION("Valid: minimal form with DOUBLE variant") {
    REQUIRE(validator.validate("D2_LJ 3.0 Ti Ti"));
  }
  SECTION("Valid: repeated block with INT") {
    // Repeated block means it can appear multiple times: "11 12 13" is valid for the second slot
    REQUIRE(validator.validate("D2_MIE 11 12.5 Ti Nb Nb Nb"));
  }
  SECTION("Invalid: missing second {STRING STRING}") {
    REQUIRE_FALSE(validator.validate("D2_LJ 11 Ti"));
  }
  SECTION("Invalid: second block has only one string") {
    REQUIRE_FALSE(validator.validate("D2_LJ 11 12 Ti"));
  }
  SECTION("Invalid: no letter in 'STRING' slot") {
    REQUIRE_FALSE(validator.validate("777 11 12 x y")); // first token must be a valid 'STRING'
  }
}

/**
 * @brief Additional edge checks for partial matches, empties, etc.
 */
TEST_CASE("Edge Cases and Partial Matches", "[blueprint]") {
  SECTION("Empty blueprint must fail everything") {
    tadah::core::BlueprintValidator emptyValidator("");
    REQUIRE_FALSE(emptyValidator.validate("anything"));
    REQUIRE_FALSE(emptyValidator.validate(""));
  }
  SECTION("Partial match does not pass full validation") {
    std::string blueprint = "INT INT";
    tadah::core::BlueprintValidator val(blueprint);
    // "10" alone is only partially matching (needs two INT).
    // The new code might detect partial match, but .validate() should fail
    REQUIRE_FALSE(val.validate("10"));
    // Full match
    REQUIRE(val.validate("10 20"));
  }
  SECTION("getCaptures for partial match fallback") {
    // Two patterns: second one might partially match better
    std::vector<std::string> bps = {"INT INT DOUBLE", "INT DOUBLE"};
    tadah::core::BlueprintValidator multiVal(bps);

    std::string textPartial = "10 3.14";
    auto captures = multiVal.getCaptures(textPartial);
    REQUIRE(captures.size() == 2); 
    // For "INT DOUBLE": captures[0]="10", captures[1]="3.14", best partial
  }
}

//------------------------------------------------------------------------------
// Additional Tests: BOOL, BOOLS
//------------------------------------------------------------------------------

TEST_CASE("Extended TYPES.BOOL checks", "[blueprint][extra]") {
  std::string blueprint = "BOOL";
  // Already tested simple "true"/"false" in older suite.
  // Now add more invalid attempts, plus stress test with whitespace or partial matches.
  std::vector<std::string> validCases = {
    "true",
    "false"
  };
  std::vector<std::string> invalidCases = {
    "0",
    "1",
    "TRUE",
    "False",
    "maybe",
    ""
  };

  SECTION("Extended valid checks") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Extended invalid checks") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

TEST_CASE("Extended TYPES.BOOLS checks", "[blueprint][extra]") {
  std::string blueprint = "{BOOL}";
  // Should allow repeated bools up to an arbitrary count, but we limit to 5 reps max in our test strings
  std::vector<std::string> validCases = {
    "true", 
    "false", 
    "true false true",
    "false true false true",
    "true true true true true"
  };
  std::vector<std::string> invalidCases = {
    "true  false   maybe", 
    "maybe", 
    "TRUE false", 
    "" 
  };

  SECTION("Extended valid checks") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Extended invalid checks") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

//------------------------------------------------------------------------------
// Additional Tests: INT, INTS
//------------------------------------------------------------------------------

TEST_CASE("Extended TYPES.INT checks", "[blueprint][extra]") {
  std::string blueprint = "INT";
  std::vector<std::string> validCases = {
    "0",
    "-0",    // same as "0" in many contexts
    "+42",
    "12345",
    "-9999"
  };
  // We expand on possible invalid ones:
  std::vector<std::string> invalidCases = {
    "+",         // sign only
    "42.0",      // decimal
    "--10",      // double negatives
    "abc",
    "",
    "+-10"
  };

  SECTION("Extended valid INT checks") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Extended invalid INT checks") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

TEST_CASE("Extended TYPES.INTS checks", "[blueprint][extra]") {
  std::string blueprint = "{INT}";
  // Up to 5 reps
  std::vector<std::string> validCases = {
    "10",
    "0 1 2 3 4",
    "-1 +2 -3 +4 -5",
    "999 123 0 -999 +20",
    "+42 +43 +44 +45 +46" // 5 repeated
  };
  std::vector<std::string> invalidCases = {
    "10.0",     // decimal
    "--5 6",    // invalid sign
    "1 x 3",    // letter
    ""          // empty
  };

  SECTION("Extended valid repeated INT checks") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Extended invalid repeated INT checks") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

//------------------------------------------------------------------------------
// Additional Tests: UINT, UINTS
//------------------------------------------------------------------------------

TEST_CASE("Extended TYPES.UINT checks", "[blueprint][extra]") {
  std::string blueprint = "UINT";
  std::vector<std::string> validCases = {
    "0",
    "1",
    "999999",
    "1234"
  };
  std::vector<std::string> invalidCases = {
    "-1",
    "+1",   // sign disallowed
    "01",   // leading zero is suspicious, but your pattern "(?:0|[1-9]\\d*)" allows "0" or "nonzeroDigit plus any digits". So "01" is invalid in the blueprint.
    "abc",
    ""
  };

  SECTION("Valid unsigned tests") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Invalid unsigned tests") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

TEST_CASE("Extended TYPES.UINTS checks", "[blueprint][extra]") {
  std::string blueprint = "{UINT}";
  // Up to 5 reps
  std::vector<std::string> validCases = {
    "0",
    "7 20 999 1234 19999",
    "1 2 3 4 5",
    "10"
  };
  std::vector<std::string> invalidCases = {
    "-1 2", 
    "01 02",   // each token invalid 
    "5 6 07",  // "07" invalid  
    ""
  };

  SECTION("Valid repeated unsigned tests") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Invalid repeated unsigned tests") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

//------------------------------------------------------------------------------
// Additional Tests: DOUBLE, DOUBLES
//------------------------------------------------------------------------------

TEST_CASE("Extended TYPES.DOUBLE checks", "[blueprint][extra]") {
  std::string blueprint = "DOUBLE";
  std::vector<std::string> validCases = {
    "0",
    "3.14",
    "-2.0",
    "2.0e+1",
    "+2.0e-3",
    "5e7",
    "-9.9999e20"
  };
  std::vector<std::string> invalidCases = {
    ".",         // no digits
    "2.",        // might be valid for some double formats, but your pattern requires at least one digit after decimal
    "e3",        // missing leading digits
    " --2.0",    // double negative sign
    "abc", 
    ""
  };

  SECTION("Extended valid float checks") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Extended invalid float checks") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

TEST_CASE("Extended TYPES.DOUBLES checks", "[blueprint][extra]") {
  std::string blueprint = "{DOUBLE}";
  // Up to 5 reps
  std::vector<std::string> validCases = {
    "2.0",
    "2.0 -4.65 0.4 3.5e2 -1e-5",
    "1 2 3 4 5",        // all valid doubles because ints also match double pattern
    "0.0 -1.0 +2.0"
  };
  std::vector<std::string> invalidCases = {
    ".",          // no leading digit
    "1. 2. 3..",  // malformed
    "abc", 
    "", 
    "2.0 .5"      // pattern doesn't allow ".5" alone; needs leading digit
  };

  SECTION("Extended valid repeated double checks") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Extended invalid repeated double checks") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

//------------------------------------------------------------------------------
// Additional Tests: STRING, STRINGS
//------------------------------------------------------------------------------

TEST_CASE("Extended TYPES.STRING checks", "[blueprint][extra]") {
  // Each token must contain at least one letter or '*'
  std::string blueprint = "STRING";
  std::vector<std::string> validCases = {
    "H",       // single char
    "*",       // single star
    "Hello", 
    "D2_LJ", 
    "abc123", 
    "some_string*ext", 
    "XYZ" 
  };
  std::vector<std::string> invalidCases = {
    "", 
    "12345",        // does not contain letter or star
    "     ",        // whitespace only
    "?!.#"         // punctuation with no letters
  };

  SECTION("Extended valid single string checks") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Extended invalid single string checks") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

TEST_CASE("Extended TYPES.STRINGS checks", "[blueprint][extra]") {
  // repeated "STRING"
  std::string blueprint = "{STRING}";
  std::vector<std::string> validCases = {
    "alpha bravo charlie",
    "* * *",
    "ABC abc 123d *", 
    "X Y Z", 
    "OneString AnotherString ThirdString FourthString FifthString"
  };
  std::vector<std::string> invalidCases = {
    "",                 // empty
    "1234 5678",        // no letter or star in either token
    "abc ??? 123"      // "???" doesn't contain a letter or star
  };

  SECTION("Extended valid repeated string checks") {
    REQUIRE(allExamplesValidate(blueprint, validCases));
  }
  SECTION("Extended invalid repeated string checks") {
    REQUIRE_FALSE(anyStringValidates(blueprint, invalidCases));
  }
}

//------------------------------------------------------------------------------
// Additional Tests: TYPE2B, focusing on the pattern: STRING {[INT | DOUBLE]} {STRING STRING}
//------------------------------------------------------------------------------

TEST_CASE("Extended TYPE2B checks", "[blueprint][extra]") {
  // The previously tested blueprint:
  std::string blueprint = "STRING {[INT | DOUBLE]} {STRING STRING}";
  tadah::core::BlueprintValidator val(blueprint);

  SECTION("Valid: multiple tokens in repeated numeric block") {
    // Repeated numeric block can combine INT/DOUBLE in any repeated order
    // e.g., "11 3.5 0 10 -1 2.0"
    REQUIRE(val.validate("D2_MIE 11 3.5 2.0 6 Ti Nb Nb Nb"));
    REQUIRE(val.validate("Descriptor 1 2 3.0 4.0 5.0 X Y"));  
  }

  SECTION("Invalid: numeric block lacking digits") {
    // The second slot is repeated [INT|DOUBLE], so "abc" is not valid
    REQUIRE_FALSE(val.validate("D2_LJ abc Ti Ti"));
    REQUIRE_FALSE(val.validate("SomeDesc 2.0 -3.0 ? Ti Ti"));
  }

  SECTION("Invalid: second repeated block (STRING STRING) has not enough tokens") {
    // We need at least 2 tokens in that repeated block. "Ti" alone is not enough.
    REQUIRE_FALSE(val.validate("D2_LJ 11 Ti"));
    REQUIRE_FALSE(val.validate("D2_LJ 99 1.5 X"));
  }

  SECTION("Edge: second repeated block with 5 pairs of strings") {
    // The blueprint pattern has “{STRING STRING},” meaning 1 block is 2 strings, can be repeated:
    // repeated block => (STRING STRING)( (STRING STRING))* 
    // e.g. "Ti Nb Al Ni Cu Fe", that’s 3 pairs => 6 tokens
    // This is valid if your code truly allows indefinite repetition
    REQUIRE(val.validate("MyDescriptor 1 -1 Ti Nb Al Cu X Y Qp Qm XEnd YEnd"));
  }
}

//------------------------------------------------------------------------------
// Additional edge coverage
//------------------------------------------------------------------------------

TEST_CASE("Additional edge coverage for partial matches", "[blueprint][extra]") {
  // Combining multiple patterns to confirm partial match logic is robust
  std::vector<std::string> bps = {"INT INT", "INT INT INT"};
  tadah::core::BlueprintValidator multiple(bps);

  SECTION("Partial match with two INT but the text has 3rd token") {
    // "10 20 30" fully matches the second pattern "INT INT INT", so .validate() => true
    REQUIRE(multiple.validate("10 20 30"));
    // "10 20 30 40" not fully matched by either pattern
    // But partial match might catch "10 20 30"
    REQUIRE_FALSE(multiple.validate("10 20 30 40"));
  }
}

// More corner cases can go here if needed, especially focusing on invalid user input.

TEST_CASE("Long numeric lines: fast path accepts underflow, rejects overflow",
          "[blueprint][long]") {
  // A potential written losslessly can carry subnormal values (strtod reports
  // them with ERANGE). The >= 2048-char numeric fast path must accept them
  // instead of falling back to std::regex; overflow is still rejected.
  auto long_line = [](const std::string &first) {
    std::string out = first;
    while (out.size() < 4096) out += " 0.123456789012345677";
    return out;
  };

  SECTION("{DOUBLE} line with subnormal and underflowing values") {
    tadah::core::BlueprintValidator validator("{DOUBLE}");
    for (const char *t : {"4.94065645841246544e-324", "1e-310", "-2.5e-320",
                          "1e-400"}) {
      INFO("first token: " << t);
      const std::string line = long_line(t);
      REQUIRE(line.size() > 2048);
      REQUIRE(validator.validate(line));
    }
  }
  SECTION("fast path taken: blueprint-independent, so a {INT} blueprint "
          "accepts a subnormal-bearing numeric line") {
    // The regex would reject the non-integer tokens outright; only the fast
    // path accepts this line, which proves the subnormal did not reject it.
    tadah::core::BlueprintValidator validator("{INT}");
    REQUIRE(validator.validate(long_line("1e-310")));
  }
  SECTION("overflow is not a valid number") {
    tadah::core::BlueprintValidator validator("{INT}");
    REQUIRE_FALSE(validator.validate(long_line("1e400")));
    REQUIRE_FALSE(validator.validate(long_line("-1e400")));
  }
}

/* Pins: the values of a list (a repeated group {...}) are separated by
 * blanks, as CoreUserConfigParser splits a configuration line into values
 * at blanks only. A comma or a '+' between two numbers is refused, with or
 * without blanks around the comma: {DOUBLE} '0.1,0.2', '0.1, 0.2',
 * '0.1 ,0.2', '0.1+0.2'; {INT} and {UINT} '1,2', '1+2'; {BOOL}
 * 'true,false'; the group {STRING INT DOUBLE DOUBLE} 'LIN 7,0.1 3' and
 * 'LIN,7 0.1 3'; 'INT {DOUBLE}' '3 1.0,2.0'. Blanks (spaces, tabs, several)
 * separate them, a '+' sign after a blank and an exponent's sign are part
 * of a number ('0.1 +0.2', '1e+5 2'). A STRING value holds a comma or a
 * '+' as part of its word ({STRING} 'a,b.tadah', 'Ti+Nb'), as the parser
 * keeps the word whole. An index list {INDEX_PATTERN} (INDEX, FIXINDEX,
 * whose readers split at commas and blanks) takes commas and blanks between
 * entries ('1,3,5', '13 14', '13, 14', '13 ,14', '1 - 4, 7', '1-10:2,12')
 * and refuses a '+' between two entries ('13+14'), an empty entry ('1,,2')
 * and a leading or trailing comma.
 * Before the fix: the elements of a repeated group were joined by the class
 * [,\s+], so these were accepted: every {DOUBLE}, {INT}, {UINT} and {BOOL}
 * comma and '+' form above, the three {STRING INT DOUBLE DOUBLE} forms,
 * 'INT {DOUBLE}' '3 1.0,2.0' and {INDEX_PATTERN} '13+14' (19 checks), and
 * the parser stored '0.1,0.2' as one value, refused when read
 * ('Conversion failed for key=..., value="0.1,0.2"'). The other refusals
 * ({STRING} '1,2' and '1+2', 'INT {DOUBLE}' '3,1.0 2.0', {INDEX_PATTERN}
 * '1,,2', '1,3,', ',1' and '1, ,2') and every valid form passed as well (a
 * pin). */
TEST_CASE("Blueprint lists are separated by blanks, as the parser splits a "
          "line",
          "[blueprint][separators]") {
  struct Bp {
    std::string blueprint;
    std::vector<std::string> valid, invalid;
  };
  const std::vector<Bp> cases = {
      {"{DOUBLE}",
       {"0.1 0.2", "0.1  0.2", "0.1\t0.2", "0.1 +0.2", "1e+5 2", "-1.5e-3"},
       {"0.1,0.2", "0.1, 0.2", "0.1 ,0.2", "0.1+0.2", "0.1 0.2,0.3",
        "1,2,3"}},
      {"{INT}", {"1 2", "-1 +2"}, {"1,2", "1+2", "1, 2"}},
      {"{UINT}", {"1 2", "80 20"}, {"1,2", "1+2", "80,20"}},
      {"{BOOL}", {"true false"}, {"true,false", "true+false"}},
      {"{STRING INT DOUBLE DOUBLE}",
       {"LIN 7 0.1 3", "LIN 7 0.1 3 LOG 5 0.2 4"},
       {"LIN 7,0.1 3", "LIN,7 0.1 3", "LIN 7 0.1+3"}},
      {"INT {DOUBLE}", {"3 1.0 2.0"}, {"3 1.0,2.0", "3,1.0 2.0"}},
      {"{STRING}", {"a,b.tadah", "Ti+Nb", "x1,2 y", "a, b"}, {"1,2", "1+2"}},
      {"{INDEX_PATTERN}",
       {"1,3,5", "1-4,7,9", "1-10:2", "13 14", "13, 14", "13 ,14",
        "1 - 4, 7", "1-10:2,12", "1 , 2 3"},
       {"13+14", "1,,2", "1,3,", ",1", "1, ,2"}},
  };
  for (const Bp &bp : cases) {
    tadah::core::BlueprintValidator v(bp.blueprint);
    for (const std::string &text : bp.valid) {
      INFO(bp.blueprint << " should take '" << text << "'");
      CHECK(v.validate(text));
    }
    for (const std::string &text : bp.invalid) {
      INFO(bp.blueprint << " should refuse '" << text << "'");
      CHECK_FALSE(v.validate(text));
    }
  }
}

/* Pins: a line of 2048 characters or more (checked without std::regex,
 * which overflows the stack on long input) is split into values at blanks
 * as a short line is: a {DOUBLE} line of numbers joined by commas
 * ('1.0,2.0,...'), by '+' or by '-' ('1.0+2.0', '1.0-2.0'), or with one
 * comma among blank-separated numbers, also at about 40000 characters, is
 * refused; one of blank-separated numbers is valid. A {STRING} line of
 * words that hold commas ('x1,2') is valid, as each word is one STRING
 * value. An {INDEX_PATTERN} line of comma-separated entries, ranges and
 * steps ('1,2,...', '1-10:2, 12, ...') is valid; one with a '+' between two
 * entries or an empty entry ('1,,2') is refused.
 * Before the fix: the numeric shortcut skipped commas and read numbers
 * glued by a sign, so the comma, '+' and '-' {DOUBLE} lines and the '+' and
 * empty-entry index lines were valid; the {STRING} shortcut split at
 * commas, so 'x1,2' was refused for its word '2'. */
TEST_CASE("Long lines: values are separated by blanks, as on a short line",
          "[blueprint][long][separators]") {
  auto repeat = [](const std::string &first, const std::string &next,
                   std::size_t size) {
    std::string out = first;
    while (out.size() < size) out += next;
    return out;
  };
  tadah::core::BlueprintValidator doubles("{DOUBLE}");
  CHECK(doubles.validate(repeat("1.0", " 2.0", 2100)));
  for (const std::string &line :
       {repeat("1.0", ",2.0", 2100), repeat("1.0", ", 2.0", 2100),
        repeat("1.0", "+2.0", 2100), repeat("1.0", "-2.0", 2100),
        repeat("1.0", " 2.0", 2100) + ",3.0",
        repeat("1.0", " 2.0", 40000) + ",3.0"}) {
    INFO("{DOUBLE} line of " << line.size() << " characters ending '"
                             << line.substr(line.size() - 12) << "'");
    REQUIRE(line.size() >= 2048);
    CHECK_FALSE(doubles.validate(line));
  }

  tadah::core::BlueprintValidator strings("{STRING}");
  const std::string comma_words = repeat("x1,2", " x1,2", 2100);
  REQUIRE(comma_words.size() >= 2048);
  CHECK(strings.validate(comma_words));

  tadah::core::BlueprintValidator indices("{INDEX_PATTERN}");
  for (const std::string &line :
       {repeat("1", ",2", 2100), repeat("1-10:2", ", 12", 2100),
        repeat("1 - 4", " 7,9", 2100), repeat("13", " 14", 2100)}) {
    INFO("{INDEX_PATTERN} line starting '" << line.substr(0, 12) << "'");
    REQUIRE(line.size() >= 2048);
    CHECK(indices.validate(line));
  }
  for (const std::string &line :
       {repeat("1", "+2", 2100), repeat("1", ",2", 2100) + ",,3",
        repeat("1", ",2", 2100) + ","}) {
    INFO("{INDEX_PATTERN} line ending '" << line.substr(line.size() - 6)
                                         << "'");
    REQUIRE(line.size() >= 2048);
    CHECK_FALSE(indices.validate(line));
  }
}
