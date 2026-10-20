// The blueprint field `retired` (BlueprintKey::retired) and the rules
// BlueprintParser enforces for it; the keys retired in 1.6.0 and the error
// scales that replace them. Parser behaviour for retired keys in files is
// in test_retired_keys.cpp.

#include "catch2/catch.hpp"

#include <tadah/core/blueprint_parser.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <toml.hpp>
#include <vector>

using namespace tadah::core;
using Catch::Matchers::Contains;

namespace {

const std::string kNoExamples = "examples = []\n";

const std::string kRetired =
    "[KEY.retired]\n"
    "reason = \"It weighted the data.\"\n"
    "replacement = \"OTHER\"\n"
    "conversion = \"OTHER = 1/sqrt(KEY).\"\n";

/* One key table in the form the blueprint uses after INHERIT expansion,
 * followed by @p tables (sub-tables of KEY, such as [KEY.retired]). */
toml::value key_table(const std::string &extra_fields,
                      const std::string &examples = "examples = [\"20\"]\n",
                      const std::string &tables = "") {
  const std::string text = "[KEY]\n"
                           "value_type = [\"DOUBLE\"]\n"
                           "value_format = [\"<double>\"]\n"
                           "value_N = [1]\n"
                           "description = \"A key.\"\n" +
                           examples + extra_fields + tables;
  std::istringstream in(text);
  return toml::find(toml::parse(in), "KEY");
}

/* The keys of task @p task (its own and its groups'), upper case, the
 * last part of their blueprint names. */
std::vector<std::string> task_keys(const std::string &task) {
  const auto &t = BlueprintParser::getTasksMapRef().at(task);
  std::vector<std::string> out;
  auto add = [&](const BlueprintKey &k) {
    const auto dot = k.keyName.rfind('.');
    std::string name =
        dot == std::string::npos ? k.keyName : k.keyName.substr(dot + 1);
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    out.push_back(name);
  };
  for (const auto &k : t.blueprintKeys) add(k);
  for (const auto &g : t.groups)
    for (const auto &k : g.blueprintKeys) add(k);
  return out;
}

bool has(const std::vector<std::string> &v, const std::string &s) {
  return std::find(v.begin(), v.end(), s) != v.end();
}

}  // namespace

TEST_CASE("Blueprint: EWEIGHT, FWEIGHT, SWEIGHT, WDBFILE_AUTO and "
          "EWEIGHT_TEMP are retired, with the reason, the replacement and "
          "the conversion",
          "[BlueprintParser][retired]") {
  const auto &keys = BlueprintParser::getKeysMapRef();
  for (const std::string key :
       {"EWEIGHT", "FWEIGHT", "SWEIGHT", "WDBFILE_AUTO", "EWEIGHT_TEMP"}) {
    INFO(key);
    const BlueprintKey &k = keys.at(key);
    REQUIRE(k.isRetired());
    CHECK_FALSE(k.isDeprecated());
    CHECK_FALSE(k.retired.reason.empty());
    CHECK_FALSE(k.retired.replacement.empty());
    CHECK_FALSE(k.retired.conversion.empty());
    /* No potential written after the retirement carries the key: it has
     * no default Context::contains() would insert, and no page or error
     * message shows how to use it. */
    CHECK(k.default_value.empty());
    CHECK(k.examples.empty());
  }
  /* The conversions of the fit's objective. */
  CHECK_THAT(keys.at("FWEIGHT").retired.conversion,
             Contains("FSIGMA = 1/sqrt(FWEIGHT)") && Contains("FORCE false"));
  CHECK_THAT(keys.at("SWEIGHT").retired.conversion,
             Contains("SSIGMA = 160.2/sqrt(SWEIGHT)") &&
                 Contains("diagonal") && Contains("STRESS false"));
  CHECK_THAT(keys.at("EWEIGHT").retired.conversion,
             Contains("ESIGMA = 1000/(N sqrt(EWEIGHT))") &&
                 Contains("N^2") && Contains("ENERGY false"));
  CHECK_THAT(keys.at("WDBFILE_AUTO").retired.conversion,
             Contains("WDBFILE") && Contains("n_d^-a"));
  CHECK_THAT(keys.at("EWEIGHT_TEMP").retired.replacement, Contains("WDBFILE"));
  /* The command-line options stay, to give the conversion. */
  CHECK(keys.at("WDBFILE_AUTO").cli_long_flag == "--wdbfile-auto");
  CHECK(keys.at("EWEIGHT_TEMP").cli_long_flag == "--eweight-temp");
  /* An ordinary key is not retired. */
  CHECK_FALSE(keys.at("LAMBDA").isRetired());
  CHECK_FALSE(keys.at("MBLOCK").isRetired());
}

TEST_CASE("Blueprint: the error scales ESIGMA, FSIGMA, SSIGMA, the energy "
          "norm ENORM and the energy switch ENERGY, with their defaults",
          "[BlueprintParser][retired]") {
  const auto &keys = BlueprintParser::getKeysMapRef();
  CHECK(keys.at("ESIGMA").default_value == std::vector<std::string>{"1"});
  CHECK(keys.at("FSIGMA").default_value ==
        (std::vector<std::string>{"0.1", "0"}));
  CHECK(keys.at("SSIGMA").default_value ==
        (std::vector<std::string>{"0.5", "0"}));
  CHECK(keys.at("ENORM").default_value == std::vector<std::string>{"ATOM"});
  CHECK(keys.at("ENORM").allowed_values ==
        (std::vector<std::string>{"ATOM", "SQRT"}));
  CHECK(keys.at("ENERGY").default_value == std::vector<std::string>{"true"});
  for (const std::string key : {"ESIGMA", "FSIGMA", "SSIGMA", "ENORM",
                                "ENERGY"}) {
    INFO(key);
    CHECK_FALSE(keys.at(key).isRetired());
    CHECK_FALSE(keys.at(key).description.empty());
    CHECK_FALSE(keys.at(key).longDescription.empty());
  }
  /* FSIGMA and SSIGMA take an absolute part and an optional relative one. */
  CHECK(keys.at("FSIGMA").value_N == std::vector<int>{-2});
  CHECK(keys.at("SSIGMA").value_N == std::vector<int>{-2});
  CHECK_THAT(keys.at("ESIGMA").description, Contains("meV/atom"));
  CHECK_THAT(keys.at("FSIGMA").description, Contains("eV/"));
  CHECK_THAT(keys.at("SSIGMA").description, Contains("GPa"));
}

TEST_CASE("BlueprintParser::buildKey reads `retired`",
          "[BlueprintParser][retired]") {
  const BlueprintKey plain = BlueprintParser::buildKey("KEY", key_table(""));
  CHECK_FALSE(plain.isRetired());
  CHECK(plain.retired.reason.empty());

  const BlueprintKey ret = BlueprintParser::buildKey(
      "KEY", key_table("cli_long_flag = \"--key\"\n", kNoExamples, kRetired));
  CHECK(ret.isRetired());
  CHECK(ret.retired.reason == "It weighted the data.");
  CHECK(ret.retired.replacement == "OTHER");
  CHECK(ret.retired.conversion == "OTHER = 1/sqrt(KEY).");
  /* The format is kept (the value of a retired key is still checked), and
   * so is a CLI flag, which gives the conversion. */
  CHECK(ret.value_type == std::vector<std::string>{"DOUBLE"});
  CHECK(ret.cli_long_flag == "--key");
}

TEST_CASE("BlueprintParser::buildKey rejects an inconsistent retired key",
          "[BlueprintParser][retired]") {
  SECTION("a default value would be inserted by Context::contains()") {
    REQUIRE_THROWS_WITH(
        BlueprintParser::buildKey(
            "KEY", key_table("default_value = [\"1\"]\n", kNoExamples,
                             kRetired)),
        Contains("Retired key KEY must not define default_value"));
  }
  SECTION("examples would show how to use a retired key") {
    REQUIRE_THROWS_WITH(
        BlueprintParser::buildKey("KEY", key_table("", "examples = [\"1\"]\n",
                                                   kRetired)),
        Contains("Retired key KEY must not define examples"));
  }
  SECTION("a key is deprecated or retired, not both") {
    REQUIRE_THROWS_WITH(
        BlueprintParser::buildKey(
            "KEY", key_table("deprecated = \"Gone.\"\n", kNoExamples,
                             kRetired)),
        Contains("KEY") && Contains("deprecated") && Contains("retired"));
  }
  SECTION("`retired` is a table of three non-empty strings") {
    for (const std::string bad :
         {std::string("retired = \"Gone.\"\n"),
          std::string("retired = { reason = \"R\", replacement = \"P\" }\n"),
          std::string("retired = { reason = \"\", replacement = \"P\", "
                      "conversion = \"C\" }\n"),
          std::string("retired = { reason = \"R\", replacement = \" \", "
                      "conversion = \"C\" }\n"),
          std::string("retired = { reason = \"R\", replacement = \"P\", "
                      "conversion = 1 }\n"),
          std::string("retired = { reason = \"R\", replacement = \"P\", "
                      "conversion = \"C\", extra = \"E\" }\n")}) {
      INFO(bad);
      REQUIRE_THROWS_WITH(
          BlueprintParser::buildKey("KEY", key_table(bad, kNoExamples)),
          Contains("'retired'") && Contains("KEY"));
    }
  }
}

TEST_CASE("BlueprintParser::findClosestKey never suggests a retired key",
          "[findClosestKey][retired]") {
  const auto &keys = BlueprintParser::getKeysMapRef();
  for (const std::string typo :
       {"EWEIGHTS", "FWEIGT", "SWEIGH", "WDBFILE_AUT", "EWEIGHT_TMP"}) {
    INFO(typo);
    const std::string suggested = BlueprintParser::findClosestKey(typo, keys);
    CHECK_FALSE(suggested.empty());
    CHECK_FALSE(keys.at(suggested).isRetired());
  }
}

TEST_CASE("Blueprint: tadah data balance shifts energies only; analysis "
          "bfunc --rescale and data dedup --threshold stay",
          "[BlueprintParser][retired]") {
  /* --rescale replaced the weights of a data file (inverse magnitudes,
   * discontinuous at its threshold); with it go the options that only
   * served it. The shared keys stay where other commands use them. */
  const auto balance = task_keys("DATA.balance");
  for (const std::string gone : {"RESCALE", "THRESHOLD", "FORCE", "STRESS"}) {
    INFO(gone);
    CHECK_FALSE(has(balance, gone));
  }
  for (const std::string kept : {"ESHIFT", "OUTFILE", "MERGE", "INDEX",
                                 "APPEND", "NUMERIC", "DBFILE"}) {
    INFO(kept);
    CHECK(has(balance, kept));
  }
  CHECK(has(task_keys("ANALYSIS.bfunc"), "RESCALE"));
  CHECK(has(task_keys("DATA.dedup"), "THRESHOLD"));
  const auto &t = BlueprintParser::getTasksMapRef().at("DATA.balance");
  for (const std::string &ex : t.examples) {
    INFO(ex);
    CHECK(ex.find("--rescale") == std::string::npos);
  }
  CHECK(t.longDescription.find("rescal") == std::string::npos);
}
