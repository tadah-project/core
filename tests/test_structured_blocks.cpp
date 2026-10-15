#include "catch2/catch.hpp"
#include <tadah/core/core_user_config_parser.h>
#include <fstream>
#include <sstream>
#include <filesystem>

using namespace tadah::core;
namespace {
const std::string block =
    "DESCRIPTOR ta DM_Embed\n"
    " PAIRS Ta Ta\n"
    " DENSITY\n  BASIS Blip5\n  FORM NONNEG\n"
    "  CENTRES 1 2\n  WIDTHS 0.4 0.5\n  AMPLITUDES 1 2\n"
    "  CUTOFF\n   TYPE Cut_Poly\n   RADIUS 6\n  ENDCUTOFF\n"
    " ENDDENSITY\n"
    " EMBEDDING\n  BASIS Gaussian\n  CENTRES 0 1\n  ETA 2 2\n"
    " ENDEMBEDDING\nENDDESCRIPTOR\n";
}

TEST_CASE("Structured blocks preserve boundaries and repeatable declarations",
          "[unified-dm][config]") {
  CoreUserConfigParser parser;
  std::istringstream input(block + "DESCRIPTOR tb FutureDescriptor\n"
                                  " UNKNOWN future values\nENDDESCRIPTOR\n");
  auto c = parser.parse(input);
  std::ostringstream out;
  c.write_potential(out, 3);
  REQUIRE(out.str().find("DESCRIPTOR ta DM_Embed") != std::string::npos);
  REQUIRE(out.str().find("DESCRIPTOR tb FutureDescriptor") != std::string::npos);
  REQUIRE(out.str().find("ENDDENSITY") != std::string::npos);
  REQUIRE(out.str().find("UNKNOWN future values") != std::string::npos);
  Context transported;
  transported.deserialize(c.serialize());
  REQUIRE(transported == c);
  std::istringstream roundtrip(out.str());
  auto reread = parser.parse(roundtrip);
  std::ostringstream written;
  reread.write_potential(written, 3);
  REQUIRE(written.str() == out.str());
}

TEST_CASE("Structured blocks append across CONFIG includes in declaration order",
          "[unified-dm][config]") {
  const auto path = std::filesystem::temp_directory_path() /
                    "tadah-unified-block-include.config";
  { std::ofstream f(path); f << block; }
  struct Cleanup { std::filesystem::path p; ~Cleanup() { std::filesystem::remove(p); } } cleanup{path};
  std::istringstream input("CONFIG " + path.string() +
                          "\nDESCRIPTOR tb Future\nENDDESCRIPTOR\n");
  auto c = CoreUserConfigParser().parse(input);
  std::ostringstream out; out << c;
  REQUIRE(out.str().find("DESCRIPTOR ta") < out.str().find("DESCRIPTOR tb"));
}

TEST_CASE("Structured blocks reject missing and mismatched boundaries",
          "[unified-dm][config]") {
  for (const auto &text : {
      "DESCRIPTOR ta DM_Embed\nDENSITY\nENDDESCRIPTOR\n",
      "DESCRIPTOR ta DM_Embed\nDENSITY\nENDCUTOFF\nENDDESCRIPTOR\n",
      "DESCRIPTOR ta DM_Embed\n", "ENDDESCRIPTOR\n"}) {
    std::istringstream input(text);
    REQUIRE_THROWS(CoreUserConfigParser().parse(input));
  }
}

TEST_CASE("Component radii survive ordinary context updates and copies", "[unified-dm][config]") {
  Context c;c.set_component_cutoffs({6.25,4.0});c.add("VERBOSE",0);
  REQUIRE(c.get<double>("RCUTMAX")==6.25);
  auto candidate=c;candidate.set_component_cutoffs({5.5});candidate.add("NUMERIC",12);
  REQUIRE(candidate.get<double>("RCUTMAX")==5.5);
  REQUIRE(c.get<double>("RCUTMAX")==6.25);
  candidate.set_component_cutoffs({});candidate.add("VERBOSE",1);
  REQUIRE(candidate.get<double>("RCUTMAX")==0);
}
