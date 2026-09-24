// Deprecated configuration keys (blueprint field `deprecated`).
//
// MPIWPCKG, MBLOCK and NBLOCK belonged to the legacy MPI training build,
// removed in 1.5.1. Files that contain them must still parse; the parser
// writes one notice per key and parse to standard error (a CONFIG include
// chain is one parse), whatever the Logger level or VERBOSE, keeps the
// value check, and never stores the key, so it cannot reappear in a
// potential written from the parsed Context.

#include "catch2/catch.hpp"

#include <tadah/core/blueprint_parser.h>
#include <tadah/core/context.h>
#include <tadah/core/core_user_config_parser.h>
#include <tadah/core/logger.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>
#include <unistd.h>

using namespace tadah::core;

namespace {

const std::vector<std::string> kMpiKeys = {"MPIWPCKG", "MBLOCK", "NBLOCK"};

/* Captures std::cerr (where the deprecation notice goes) and std::cout
 * (where the Logger prints) and sets the Logger level; restores the streams
 * and the level afterwards. A VERBOSE line in a parsed file changes the
 * Logger level too, so the restore covers it. */
class StreamCapture {
public:
  explicit StreamCapture(LogLevel level)
      : old_level_(Logger::instance().getLogLevel()),
        old_out_(std::cout.rdbuf(out_.rdbuf())),
        old_err_(std::cerr.rdbuf(err_.rdbuf())) {
    Logger::instance().setLogLevel(level);
  }
  ~StreamCapture() {
    std::cout.rdbuf(old_out_);
    std::cerr.rdbuf(old_err_);
    Logger::instance().setLogLevel(old_level_);
  }
  StreamCapture(const StreamCapture &) = delete;
  StreamCapture &operator=(const StreamCapture &) = delete;

  /* Lines of the captured standard error that contain `needle`. */
  std::vector<std::string> err_lines_with(const std::string &needle) const {
    return lines_with(err_.str(), needle);
  }
  /* Lines of the captured standard output that contain `needle`. */
  std::vector<std::string> out_lines_with(const std::string &needle) const {
    return lines_with(out_.str(), needle);
  }

private:
  static std::vector<std::string> lines_with(const std::string &text,
                                             const std::string &needle) {
    std::vector<std::string> out;
    std::istringstream in(text);
    for (std::string line; std::getline(in, line);)
      if (line.find(needle) != std::string::npos) out.push_back(line);
    return out;
  }

  std::ostringstream out_;
  std::ostringstream err_;
  LogLevel old_level_;
  std::streambuf *old_out_;
  std::streambuf *old_err_;
};

std::string deprecation_line(const std::string &key) {
  return "Deprecated key " + key + " is ignored";
}

/* A scratch directory unique to this process (ctest runs every test case
 * in its own process, possibly concurrently), removed with its files when
 * the object goes out of scope. */
class ScratchDir {
public:
  ScratchDir()
      : dir_(std::filesystem::temp_directory_path() /
             ("tadah_core_deprecated_keys_" + std::to_string(::getpid()))) {
    std::filesystem::create_directories(dir_);
  }
  ~ScratchDir() {
    std::error_code ec;
    std::filesystem::remove_all(dir_, ec);
  }
  ScratchDir(const ScratchDir &) = delete;
  ScratchDir &operator=(const ScratchDir &) = delete;

  const std::filesystem::path &path() const { return dir_; }

  std::filesystem::path write_file(const std::string &name,
                                   const std::string &body) const {
    const auto p = dir_ / name;
    std::ofstream(p) << body;
    return p;
  }

private:
  std::filesystem::path dir_;
};

std::string serialized(const Context &c) {
  std::ostringstream os;
  os << c;
  return os.str();
}

bool has_key_line(const std::string &text, const std::string &key) {
  std::istringstream in(text);
  for (std::string line; std::getline(in, line);) {
    std::istringstream ls(line);
    std::string first;
    if ((ls >> first) && first == key) return true;
  }
  return false;
}

/* The top file sets MBLOCK and includes a file that sets MBLOCK again,
 * NBLOCK and WATOM: one notice per key, and the ordinary keys of both
 * files are kept. */
void check_include_chain(const std::filesystem::path &top) {
  StreamCapture cap(LogLevel::WARNING);
  Context c;
  REQUIRE_NOTHROW(c = CoreUserConfigParser().parse(top.string()));
  CHECK(cap.err_lines_with(deprecation_line("MBLOCK")).size() == 1);
  CHECK(cap.err_lines_with(deprecation_line("NBLOCK")).size() == 1);
  CHECK_FALSE(c.contains("MBLOCK"));
  CHECK_FALSE(c.contains("NBLOCK"));
  CHECK(c.get<std::string>("ATOM") == "Ta");
  CHECK(c.get<double>("WATOM") == Approx(73.0));
}

} // namespace

TEST_CASE("Config parser: the legacy MPI keys parse, warn once each on "
          "stderr and are not stored",
          "[config][deprecated]") {
  /* The Logger is at WARNING, so a notice sent through it would reach
   * standard output: it must appear on standard error only. */
  StreamCapture cap(LogLevel::WARNING);
  Context c;
  REQUIRE_NOTHROW(c = CoreUserConfigParser().parse(std::vector<std::string>{
                      "ATOM Ta", "MPIWPCKG 50", "MBLOCK 64", "NBLOCK 64",
                      "MBLOCK 32   # a second line for the same key"}));

  CHECK(c.get<std::string>("ATOM") == "Ta");
  for (const auto &key : kMpiKeys) {
    INFO(key);
    const auto warned = cap.err_lines_with(deprecation_line(key));
    REQUIRE(warned.size() == 1);
    CHECK_THAT(warned[0], Catch::Matchers::StartsWith("WARNING: ") &&
                              Catch::Matchers::Contains("1.5.1") &&
                              Catch::Matchers::Contains("TADAH_BUILD_MPI"));
    CHECK(cap.out_lines_with(deprecation_line(key)).empty());
    CHECK_FALSE(c.contains(key));
  }
}

TEST_CASE("Config parser: the deprecation notice ignores the Logger level and "
          "VERBOSE",
          "[config][deprecated]") {
  /* pair_style tadah leaves the Logger at ERROR, and a VERBOSE 0 line
   * lowers it while the file is parsed: neither may hide the notice. */
  SECTION("Logger at ERROR, as in LAMMPS") {
    StreamCapture cap(LogLevel::ERROR);
    REQUIRE_NOTHROW(CoreUserConfigParser().parse(
        std::vector<std::string>{"ATOM Ta", "MBLOCK 64"}));
    CHECK(cap.err_lines_with(deprecation_line("MBLOCK")).size() == 1);
  }
  SECTION("VERBOSE 0 on an earlier line") {
    StreamCapture cap(LogLevel::WARNING);
    REQUIRE_NOTHROW(CoreUserConfigParser().parse(std::vector<std::string>{
        "VERBOSE 0", "ATOM Ta", "MPIWPCKG 50", "NBLOCK 64"}));
    CHECK(cap.err_lines_with(deprecation_line("MPIWPCKG")).size() == 1);
    CHECK(cap.err_lines_with(deprecation_line("NBLOCK")).size() == 1);
  }
}

TEST_CASE("Config parser: a deprecated key never reaches a written potential",
          "[config][deprecated]") {
  /* A potential file is the Context streamed with operator<< (see
   * TrainEngine::dumpPotFile). contains() inserts blueprint defaults, so a
   * deprecated key must have none, or it would be written back. */
  SECTION("parsed from a file") {
    StreamCapture cap(LogLevel::WARNING);
    Context c = CoreUserConfigParser().parse(std::vector<std::string>{
        "ATOM Ta", "WATOM 73", "MPIWPCKG 50", "MBLOCK 64", "NBLOCK 64"});
    for (const auto &key : kMpiKeys) CHECK_FALSE(c.contains(key));
    const std::string pot = serialized(c);
    CHECK(has_key_line(pot, "ATOM"));
    for (const auto &key : kMpiKeys) {
      INFO(key << " in:\n" << pot);
      CHECK_FALSE(has_key_line(pot, key));
    }
  }
  SECTION("queried on a Context that never had it") {
    Context c;
    c.add("ATOM", "Ta");
    for (const auto &key : kMpiKeys) CHECK_FALSE(c.contains(key));
    const std::string pot = serialized(c);
    for (const auto &key : kMpiKeys) {
      INFO(key << " in:\n" << pot);
      CHECK_FALSE(has_key_line(pot, key));
    }
  }
  SECTION("the blueprint gives them no default") {
    const auto &keys = BlueprintParser::getKeysMapRef();
    for (const auto &key : kMpiKeys) {
      INFO(key);
      CHECK(keys.at(key).default_value.empty());
    }
  }
}

TEST_CASE("Config parser: a CONFIG include chain reports a deprecated key once",
          "[config][deprecated]") {
  std::filesystem::path scratch;
  {
    const ScratchDir dir;
    scratch = dir.path();
    const auto sub =
        dir.write_file("sub.cfg", "MBLOCK 32\nNBLOCK 8\nWATOM 73\n");
    const auto top = dir.write_file(
        "top.cfg", "ATOM Ta\nMBLOCK 64\nCONFIG " + sub.string() + "\n");
    check_include_chain(top);
  }
  // The files of the test do not outlive it.
  CHECK_FALSE(std::filesystem::exists(scratch));
}

TEST_CASE("Config parser: every parse reports its deprecated keys, also after "
          "a parse that failed",
          "[config][deprecated]") {
  const std::vector<std::string> ok = {"ATOM Ta", "MBLOCK 64"};
  StreamCapture cap(LogLevel::WARNING);

  REQUIRE_NOTHROW(CoreUserConfigParser().parse(ok));
  CHECK(cap.err_lines_with(deprecation_line("MBLOCK")).size() == 1);

  // A failing parse (unknown key after a deprecated one) must not leave
  // the parser believing it is still inside that parse.
  REQUIRE_THROWS_WITH(CoreUserConfigParser().parse(std::vector<std::string>{
                          "MBLOCK 64", "NOT_A_TADAH_KEY 1"}),
                      Catch::Matchers::Contains("NOT_A_TADAH_KEY"));
  CHECK(cap.err_lines_with(deprecation_line("MBLOCK")).size() == 2);

  REQUIRE_NOTHROW(CoreUserConfigParser().parse(ok));
  CHECK(cap.err_lines_with(deprecation_line("MBLOCK")).size() == 3);
}

TEST_CASE("Config parser: a deprecated key's value is still checked",
          "[config][deprecated]") {
  StreamCapture cap(LogLevel::WARNING);
  REQUIRE_THROWS_WITH(CoreUserConfigParser().parse(std::vector<std::string>{
                          "MBLOCK sixty-four"}),
                      Catch::Matchers::Contains("Invalid schema for key: MBLOCK"));
  REQUIRE_THROWS_WITH(CoreUserConfigParser().parse(std::vector<std::string>{
                          "NBLOCK -1"}),
                      Catch::Matchers::Contains("Invalid schema for key: NBLOCK"));
}

TEST_CASE("BlueprintParser::findClosestKey never suggests a deprecated key",
          "[findClosestKey][deprecated]") {
  const auto &keys = BlueprintParser::getKeysMapRef();
  for (const std::string typo : {"MBLOK", "NBLOK", "MPIWPCK"}) {
    INFO(typo);
    const std::string suggested = BlueprintParser::findClosestKey(typo, keys);
    CHECK_FALSE(suggested.empty());
    for (const auto &key : kMpiKeys) CHECK(suggested != key);
  }
}
