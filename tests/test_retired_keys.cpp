// Retired configuration keys (blueprint field `retired`) in files and in
// the Context: EWEIGHT, FWEIGHT, SWEIGHT, WDBFILE_AUTO and EWEIGHT_TEMP,
// which every potential of Tadah! 1.0.0 to 1.5.0 carries.
//
// A file that has them still parses (a potential keeps loading): the
// parser checks the value, records the line on the Context it returns
// (Context::retired_keys()) instead of storing the key, and gives one
// notice per parse that lists them. Context::add() refuses the key, and a
// fit refuses a configuration that records one, with the conversion
// (retired_keys_refusal()).

#include "catch2/catch.hpp"

#include <tadah/core/blueprint_parser.h>
#include <tadah/core/context.h>
#include <tadah/core/core_user_config_parser.h>
#include <tadah/core/key_notice.h>
#include <tadah/core/logger.h>
#include <tadah/core/retired_keys.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>
#include <unistd.h>

using namespace tadah::core;
using Catch::Matchers::Contains;
using Catch::Matchers::StartsWith;

namespace {

/* A scratch directory unique to this process, removed with its files. */
class ScratchDir {
public:
  ScratchDir()
      : dir_(std::filesystem::temp_directory_path() /
             ("tadah_core_retired_keys_" + std::to_string(::getpid()))) {
    std::filesystem::create_directories(dir_);
  }
  ~ScratchDir() {
    std::error_code ec;
    std::filesystem::remove_all(dir_, ec);
  }
  ScratchDir(const ScratchDir &) = delete;
  ScratchDir &operator=(const ScratchDir &) = delete;

  std::string write_file(const std::string &name,
                         const std::string &body) const {
    const auto p = dir_ / name;
    std::ofstream(p) << body;
    return p.string();
  }

private:
  std::filesystem::path dir_;
};

/* Captures std::cerr; restores it afterwards. */
class ErrCapture {
public:
  ErrCapture() : old_(std::cerr.rdbuf(err_.rdbuf())) {}
  ~ErrCapture() { std::cerr.rdbuf(old_); }
  ErrCapture(const ErrCapture &) = delete;
  ErrCapture &operator=(const ErrCapture &) = delete;
  std::vector<std::string> lines_with(const std::string &needle) const {
    std::vector<std::string> out;
    std::istringstream in(err_.str());
    for (std::string line; std::getline(in, line);)
      if (line.find(needle) != std::string::npos) out.push_back(line);
    return out;
  }

private:
  std::ostringstream err_;
  std::streambuf *old_;
};

/* The lines of a 1.5.0 potential that set the retired keys, with the keys
 * of its model around them. */
const std::string kOldPotential = "ATOM           Ta\n"
                                  "EWEIGHT        1.0\n"
                                  "FWEIGHT        100   # forces\n"
                                  "LAMBDA         0\n"
                                  "SWEIGHT        0\n"
                                  "WDBFILE_AUTO   0.0\n";

bool has_key_line(const std::string &text, const std::string &key) {
  std::istringstream in(text);
  for (std::string line; std::getline(in, line);) {
    std::istringstream ls(line);
    std::string first;
    if ((ls >> first) && first == key) return true;
  }
  return false;
}

std::vector<std::string> keys_of(const std::vector<RetiredKeyLine> &lines) {
  std::vector<std::string> out;
  for (const auto &l : lines) out.push_back(l.key);
  return out;
}

}  // namespace

TEST_CASE("Config parser: a retired key line is checked, recorded on the "
          "result and not stored",
          "[config][retired]") {
  IgnoredKeyNoticeCapture notices;
  Context c;
  REQUIRE_NOTHROW(c = CoreUserConfigParser().parse(std::vector<std::string>{
                      "ATOM Ta", "EWEIGHT 1.0", "FWEIGHT 100  # forces",
                      "LAMBDA 0", "SWEIGHT 0", "WDBFILE_AUTO 0.0",
                      "EWEIGHT_TEMP 300"}));
  CHECK(c.get<std::string>("ATOM") == "Ta");
  for (const std::string key :
       {"EWEIGHT", "FWEIGHT", "SWEIGHT", "WDBFILE_AUTO", "EWEIGHT_TEMP"}) {
    INFO(key);
    CHECK_FALSE(c.has_stored(key));
    CHECK_FALSE(c.contains(key));  // no default either
  }
  const std::vector<RetiredKeyLine> r = c.retired_keys();
  REQUIRE(keys_of(r) == (std::vector<std::string>{"EWEIGHT", "FWEIGHT",
                                                  "SWEIGHT", "WDBFILE_AUTO",
                                                  "EWEIGHT_TEMP"}));
  CHECK(r[0].line == 2);
  CHECK(r[0].values == std::vector<std::string>{"1.0"});
  CHECK(r[1].line == 3);
  CHECK(r[1].values == std::vector<std::string>{"100"});
  CHECK(r[1].text == "FWEIGHT 100  # forces");
  CHECK(r[4].line == 7);
  CHECK(r[4].source.empty());
}

TEST_CASE("Config parser: a retired key's value is still checked",
          "[config][retired]") {
  REQUIRE_THROWS_WITH(CoreUserConfigParser().parse(std::vector<std::string>{
                          "EWEIGHT heavy"}),
                      Contains("Invalid schema for key: EWEIGHT"));
}

TEST_CASE("Config parser: one notice per parse lists the retired keys of "
          "the file, with their lines",
          "[config][retired]") {
  const ScratchDir dir;
  const std::string pot = dir.write_file("old.tadah", kOldPotential);
  const std::string plain = dir.write_file("new.tadah", "ATOM Ta\n");

  SECTION("under a capture, as pair_style tadah loads a potential") {
    IgnoredKeyNoticeCapture cap;
    CoreUserConfigParser().parse(pot);
    REQUIRE(cap.notices().size() == 1);
    const std::string &n = cap.notices()[0];
    CHECK_THAT(n, StartsWith("Retired keys"));
    CHECK_THAT(n, Contains("EWEIGHT (line 2 of " + pot + ")") &&
                      Contains("FWEIGHT (line 3 of " + pot + ")") &&
                      Contains("SWEIGHT (line 5 of " + pot + ")") &&
                      Contains("WDBFILE_AUTO (line 6 of " + pot + ")"));
    CHECK_THAT(n, Contains("ignored") && Contains("refuse") &&
                      Contains("tadah explain"));
    /* Every load gives its notice: a second parse of the same file too. */
    CoreUserConfigParser().parse(pot);
    CHECK(cap.notices().size() == 2);
    /* A file without retired keys gives none. */
    CoreUserConfigParser().parse(plain);
    CHECK(cap.notices().size() == 2);
  }
  SECTION("on standard error, whatever the Logger level") {
    const LogLevel old = Logger::instance().getLogLevel();
    Logger::instance().setLogLevel(LogLevel::ERROR);
    ErrCapture err;
    CoreUserConfigParser().parse(pot);
    Logger::instance().setLogLevel(old);
    const auto lines = err.lines_with("Retired keys");
    REQUIRE(lines.size() == 1);
    CHECK_THAT(lines[0], StartsWith("WARNING: Retired keys"));
  }
  SECTION("one retired key") {
    IgnoredKeyNoticeCapture cap;
    CoreUserConfigParser().parse(std::vector<std::string>{"EWEIGHT 1"});
    REQUIRE(cap.notices().size() == 1);
    CHECK_THAT(cap.notices()[0],
               StartsWith("Retired key EWEIGHT (line 1) is ignored"));
  }
}

TEST_CASE("Config parser: a retired key of a CONFIG include is recorded with "
          "its file, and the parse gives one notice",
          "[config][retired]") {
  const ScratchDir dir;
  const std::string sub = dir.write_file("old.tadah", kOldPotential);
  const std::string top = dir.write_file(
      "top.train", "CONFIG " + sub + "\nEWEIGHT_TEMP 500\nDBFILE db.tadah\n");
  IgnoredKeyNoticeCapture cap;
  const Context c = CoreUserConfigParser().parse(top);
  const auto r = c.retired_keys();
  REQUIRE(keys_of(r) == (std::vector<std::string>{"EWEIGHT", "FWEIGHT",
                                                  "SWEIGHT", "WDBFILE_AUTO",
                                                  "EWEIGHT_TEMP"}));
  CHECK(r[0].source == sub);
  CHECK(r[4].source == top);
  CHECK(r[4].line == 2);
  REQUIRE(cap.notices().size() == 1);
  CHECK_THAT(cap.notices()[0], Contains("EWEIGHT_TEMP (line 2 of " + top));
}

TEST_CASE("Context: the retired lines travel with copies, merges and "
          "serialisation, and no writer writes them",
          "[context][retired]") {
  IgnoredKeyNoticeCapture quiet;
  const Context c = CoreUserConfigParser().parse(std::vector<std::string>{
      "ATOM Ta", "EWEIGHT 1.0", "WDBFILE_AUTO 0.5"});
  const Context copy = c;
  CHECK(keys_of(copy.retired_keys()) ==
        (std::vector<std::string>{"EWEIGHT", "WDBFILE_AUTO"}));

  Context other;
  other.add("LAMBDA", 0.1);
  /* Either side of a merge keeps them, and a merge with itself adds no
   * second copy. */
  CHECK(Context::merge(other, c).retired_keys().size() == 2);
  CHECK(Context::merge(c, other).retired_keys().size() == 2);
  CHECK(Context::merge(c, c).retired_keys().size() == 2);
  CHECK(Context::merge(c, c, true, Context::BlockMerge::Append)
            .retired_keys()
            .size() == 2);

  Context back;
  back.deserialize(c.serialize());
  const auto r = back.retired_keys();
  REQUIRE(r.size() == 2);
  CHECK(r[0].key == "EWEIGHT");
  CHECK(r[0].values == std::vector<std::string>{"1.0"});
  CHECK(r[1].line == 3);

  std::ostringstream as_text, as_pot;
  as_text << c;
  c.write_potential(as_pot, 17);
  for (const std::string key : {"EWEIGHT", "WDBFILE_AUTO"}) {
    INFO(key);
    CHECK_FALSE(has_key_line(as_text.str(), key));
    CHECK_FALSE(has_key_line(as_pot.str(), key));
  }
  CHECK(has_key_line(as_pot.str(), "ATOM"));
  CHECK(as_text.str().find("@") == std::string::npos);
}

TEST_CASE("Context::add refuses a retired key with its conversion, and a "
          "read of one names it as retired",
          "[context][retired]") {
  Context c;
  REQUIRE_THROWS_WITH(c.add("EWEIGHT", 1.0),
                      Contains("EWEIGHT") && Contains("retired") &&
                          Contains("ESIGMA = 1000/(N sqrt(EWEIGHT))"));
  REQUIRE_THROWS_WITH(c.add("wdbfile_auto", "0.5"),
                      Contains("WDBFILE_AUTO") && Contains("retired") &&
                          Contains("n_d^-a"));
  CHECK_FALSE(c.has_stored("EWEIGHT"));
  REQUIRE_THROWS_WITH(c.get<double>("FWEIGHT"),
                      Contains("FWEIGHT") && Contains("retired") &&
                          Contains("FSIGMA = 1/sqrt(FWEIGHT)"));
}

TEST_CASE("The error scales, ENORM and ENERGY are keys of a configuration",
          "[config][retired]") {
  const Context c = CoreUserConfigParser().parse(std::vector<std::string>{
      "ESIGMA 0.5", "FSIGMA 0.2 0.05", "SSIGMA 1", "ENORM sqrt",
      "ENERGY false"});
  CHECK(c.get<double>("ESIGMA") == 0.5);
  CHECK(c.size("FSIGMA") == 2);
  CHECK(c.get<double>("FSIGMA", 1) == 0.05);
  CHECK(c.size("SSIGMA") == 1);
  CHECK(c.get<std::string>("ENORM") == "sqrt");
  CHECK_FALSE(c.get<bool>("ENERGY"));
  REQUIRE_THROWS_WITH(CoreUserConfigParser().parse(std::vector<std::string>{
                          "ENORM TOTAL"}),
                      Contains("ENORM") && Contains("ATOM, SQRT"));
  REQUIRE_THROWS(CoreUserConfigParser().parse(std::vector<std::string>{
      "FSIGMA 0.1 0 0"}));
  /* The defaults: written into every potential a fit writes. */
  Context d;
  CHECK(d.get<double>("ESIGMA") == 1.0);
  CHECK(d("FSIGMA") == (std::vector<std::string>{"0.1", "0"}));
  CHECK(d("SSIGMA") == (std::vector<std::string>{"0.5", "0"}));
  CHECK(d.get<std::string>("ENORM") == "ATOM");
  CHECK(d.get<bool>("ENERGY"));
}

TEST_CASE("retired_keys_refusal names each retired line and its conversion",
          "[retired]") {
  IgnoredKeyNoticeCapture quiet;
  const ScratchDir dir;
  const std::string pot = dir.write_file("old.tadah", kOldPotential);
  const Context c = CoreUserConfigParser().parse(pot);
  const std::string msg = retired_keys_refusal(c.retired_keys(), "tadah train");
  CHECK_THAT(msg, StartsWith("tadah train: "));
  CHECK_THAT(msg, Contains("EWEIGHT 1.0 (line 2 of " + pot + ")") &&
                      Contains("FWEIGHT 100 (line 3 of " + pot + ")") &&
                      Contains("SWEIGHT 0 (line 5 of " + pot + ")") &&
                      Contains("WDBFILE_AUTO 0.0 (line 6 of " + pot + ")"));
  const auto &keys = BlueprintParser::getKeysMapRef();
  for (const std::string key :
       {"EWEIGHT", "FWEIGHT", "SWEIGHT", "WDBFILE_AUTO"}) {
    INFO(key);
    CHECK_THAT(msg, Contains(keys.at(key).retired.conversion));
  }
  /* Each key's conversion once, however many lines set it. */
  const Context twice = CoreUserConfigParser().parse(
      std::vector<std::string>{"FWEIGHT 1", "FWEIGHT 2"});
  const std::string m2 = retired_keys_refusal(twice.retired_keys(), "A fit");
  const std::string conv = keys.at("FWEIGHT").retired.conversion;
  CHECK(m2.find(conv) != std::string::npos);
  CHECK(m2.find(conv, m2.find(conv) + 1) == std::string::npos);

  REQUIRE_THROWS_WITH(require_no_retired_keys(c, "A fit"),
                      StartsWith("A fit: ") && Contains("EWEIGHT 1.0"));
  Context fresh;
  fresh.add("ATOM", "Ta");
  REQUIRE_NOTHROW(require_no_retired_keys(fresh, "A fit"));
}
