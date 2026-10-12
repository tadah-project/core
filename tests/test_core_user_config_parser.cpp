#include "catch2/catch.hpp"
#include <tadah/core/blueprint_parser.h>
#include <tadah/core/context.h>
#include <tadah/core/core_user_config_parser.h>
#include <tadah/core/logger.h>
#include <dirent.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <istream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>
#include <unistd.h>

using namespace tadah::core;

TEST_CASE( "Testing Context read()", "[config_read]" ) {
  std::string path = "tests_data/valid_configs";
  DIR* dir = opendir(path.c_str());
  if (dir) {
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string filename(entry->d_name);
      if (filename != "." && filename != "..") {
        REQUIRE_NOTHROW(CoreUserConfigParser().parse(path+"/"+filename));
      }
    }
    closedir(dir);
  } else {
    std::cerr << "Failed to open directory." << std::endl;
  }

  path = "tests_data/invalid_configs";
  dir = opendir(path.c_str());
  if (dir) {
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string filename(entry->d_name);
      if (filename != "." && filename != "..") {
        std::cerr << "Testing: " << path+"/"+filename << std::endl;
        REQUIRE_THROWS(CoreUserConfigParser().parse(path+"/"+filename));
      }
    }
    closedir(dir);
  } else {
    std::cerr << "Failed to open directory." << std::endl;
  }

  Context c = CoreUserConfigParser().parse("tests_data/valid_configs/valid_config1");
  REQUIRE(c.size("DBFILE")==4);

  REQUIRE_THROWS(CoreUserConfigParser().parse("some_random_non_existing_config"));
}
TEST_CASE( "Testing Context add", "[config]" ) {
  std::string path = "tests_data/valid_configs";
  DIR* dir = opendir(path.c_str());
  if (dir) {
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string filename(entry->d_name);
      if (filename != "." && filename != "..") {
        REQUIRE_NOTHROW(CoreUserConfigParser().parse(path+"/"+filename));
        Context c1 = CoreUserConfigParser().parse(path+"/"+filename);
        Context c2 = CoreUserConfigParser().parse(path+"/"+filename);
        REQUIRE(c1==c2);
      }
    }
    closedir(dir);
  } else {
    std::cerr << "Failed to open directory." << std::endl;
  }
}
TEST_CASE("read() with empty lines and comments") {
    // We'll create a temporary file containing some blank lines and commented lines
    const char* filename = "test_empty_comment.cfg";
    {
        std::ofstream ofs(filename);
        ofs << "\n";               // empty line
        ofs << "# This is a comment line\n";
        ofs << "INIT2B true   # key plus inline comment\n";
        ofs << "\n";               // another blank line
    }

    // Now read it
    Context c = CoreUserConfigParser().parse(filename);
    // We expect "INIT2B true" was read
    REQUIRE(c.get<bool>("INIT2B") == true);

    // Cleanup
    std::remove(filename);
}

// ---------------------------------------------------------------------------
// CoreUserConfigParser::refuse_keys({KEY -> reason})
//
// A parser told to refuse a key (upper-case in the map) throws a
// std::runtime_error at the line that sets it, whatever its letter case.
// The message names the key in upper case, says it is "not accepted",
// names the line ("line N", counting every line of the file, comments and
// blank lines included) and the file, and gives the caller's reason. A
// CONFIG include inherits the refusal; the error then names the included
// file and its line. HPO refuses FORCE and STRESS in the --validation file
// this way: what validation predicts follows PC_FRMSE / PC_SRMSE, and what
// training fits follows the configuration file and -F/-S.
//
// These cases were committed before CoreUserConfigParser::refuse_keys
// existed. They reached it through a detection trait then, so the file
// compiled and every refuse_keys case failed at run time; now that the
// method exists they call it directly. Before it existed, a FORCE or STRESS
// line in HPO's --validation file was silently ignored.
// ---------------------------------------------------------------------------
namespace {

/* The reasons a caller gives. Neither contains a refused key in upper case,
 * "not accepted" or "line", so they cannot satisfy those checks for it. */
const std::string kForceReason =
    "what validation predicts follows PC_FRMSE, not this file";
const std::string kStressReason =
    "what validation predicts follows PC_SRMSE, not this file";

/* A scratch directory unique to this process (ctest runs every test case in
 * a process of its own, possibly concurrently), removed with its files when
 * the object goes out of scope. Its path has no upper-case FORCE or STRESS
 * in it, so a file name in a message cannot satisfy a check for the key. */
class RefuseScratch {
public:
  RefuseScratch()
      : dir_(std::filesystem::temp_directory_path() /
             ("tadah_core_refuse_keys_" + std::to_string(::getpid()))) {
    std::error_code ec;
    std::filesystem::remove_all(dir_, ec);
    std::filesystem::create_directories(dir_);
  }
  ~RefuseScratch() {
    std::error_code ec;
    std::filesystem::remove_all(dir_, ec);
  }
  RefuseScratch(const RefuseScratch &) = delete;
  RefuseScratch &operator=(const RefuseScratch &) = delete;

  /* Writes @p body to <dir>/<name>; returns the path as a string. */
  std::string write(const std::string &name, const std::string &body) const {
    const auto p = dir_ / name;
    std::ofstream os(p);
    os << body;
    os.close();
    REQUIRE_FALSE(os.fail());
    return p.string();
  }

  /* The path of <dir>/<name>, written or not. */
  std::string path(const std::string &name) const {
    return (dir_ / name).string();
  }

private:
  std::filesystem::path dir_;
};

/* The message of the std::runtime_error that @p parse_call throws; fails the
 * case when it throws none. Another exception type propagates (and fails the
 * case): the contract is std::runtime_error. */
template <class F>
std::string runtime_error_of(F &&parse_call, const std::string &what) {
  try {
    (void)parse_call();
  } catch (const std::runtime_error &e) {
    return e.what();
  }
  FAIL(what << " did not throw std::runtime_error");
  return {};
}

/* Whether @p msg names line @p n: "line <n>" not followed by another digit
 * (so line 1 is not found in "line 12"). */
bool names_line(const std::string &msg, std::size_t n) {
  const std::string needle = "line " + std::to_string(n);
  for (auto pos = msg.find(needle); pos != std::string::npos;
       pos = msg.find(needle, pos + 1)) {
    const auto end = pos + needle.size();
    if (end == msg.size() ||
        !std::isdigit(static_cast<unsigned char>(msg[end])))
      return true;
  }
  return false;
}

/* The checks every refusal shares: the key in upper case, "not accepted",
 * the line, the source (when not empty) and the reason. */
void check_refusal(const std::string &msg, const std::string &key,
                   std::size_t line, const std::string &source,
                   const std::string &reason) {
  INFO("message: " << msg);
  CHECK_THAT(msg, Catch::Matchers::Contains(key));
  CHECK_THAT(msg, Catch::Matchers::Contains("not accepted"));
  CHECK(names_line(msg, line));
  if (!source.empty()) CHECK_THAT(msg, Catch::Matchers::Contains(source));
  CHECK_THAT(msg, Catch::Matchers::Contains(reason));
}

} // namespace

// Pins: a refused key in a file is an error at its own line (comment and
// blank lines count), in any letter case, with an inline comment, for the
// value false as well as true, naming the upper-case key, "not accepted",
// the file and that key's reason (not another key's).
// Before refuse_keys existed this case failed before any parse. Matching
// the key case-sensitively, or counting only the non-blank, non-comment
// lines, makes it fail.
TEST_CASE("Config parser: refuse_keys refuses a key at its line, naming the "
          "key, the line, the file and the reason",
          "[config][refuse_keys]") {
  RefuseScratch dir;
  CoreUserConfigParser parser;
  parser.refuse_keys({{"FORCE", kForceReason}, {"STRESS", kStressReason}});

  SECTION("an upper-case key after a comment line and a blank line") {
    const std::string file = dir.write("val_upper.cfg",
                                       "# validation set\n"    // line 1
                                       "\n"                    // line 2
                                       "ATOM Ta\n"             // line 3
                                       "# the refused key\n"   // line 4
                                       "FORCE true\n"          // line 5
                                       "WATOM 73\n");          // line 6
    const std::string msg =
        runtime_error_of([&] { return parser.parse(file); }, "parse " + file);
    check_refusal(msg, "FORCE", 5, file, kForceReason);
  }
  SECTION("a lower-case key with an inline comment, value false") {
    const std::string file = dir.write("val_lower.cfg",
                                       "atom Ta\n"                        // 1
                                       "force false   # energies only\n"  // 2
                                       "watom 73\n");                     // 3
    const std::string msg =
        runtime_error_of([&] { return parser.parse(file); }, "parse " + file);
    check_refusal(msg, "FORCE", 2, file, kForceReason);
  }
  SECTION("a mixed-case STRESS gives the STRESS reason") {
    const std::string file = dir.write("val_mixed.cfg",
                                       "ATOM Ta\n"      // line 1
                                       "WATOM 73\n"     // line 2
                                       "\n"             // line 3
                                       "Stress true\n"  // line 4
                                       "INIT2B true\n");
    const std::string msg =
        runtime_error_of([&] { return parser.parse(file); }, "parse " + file);
    check_refusal(msg, "STRESS", 4, file, kStressReason);
    CHECK_THAT(msg, !Catch::Matchers::Contains(kForceReason));
  }
}

// Pins: CONFIG includes inherit the refused keys, at every depth; the error
// names the included file and the line of that file, and a refused key
// after an include still names the including file and its own line.
// Before refuse_keys existed this case failed before any parse. Dropping
// the refusal from the parser of a CONFIG include makes it fail.
TEST_CASE("Config parser: refuse_keys reaches into CONFIG includes",
          "[config][refuse_keys]") {
  RefuseScratch dir;
  CoreUserConfigParser parser;
  parser.refuse_keys({{"FORCE", kForceReason}});

  SECTION("a refused key inside an included file names that file and line") {
    const std::string inner = dir.write("inner.cfg",
                                        "# included settings\n"  // line 1
                                        "\n"                     // line 2
                                        "WATOM 73\n"             // line 3
                                        "FORCE true\n");         // line 4
    const std::string outer = dir.write("outer.cfg",
                                        "ATOM Ta\n"                // line 1
                                        "CONFIG " + inner + "\n"   // line 2
                                        "INIT2B true\n");          // line 3
    const std::string msg = runtime_error_of(
        [&] { return parser.parse(outer); }, "parse " + outer);
    check_refusal(msg, "FORCE", 4, inner, kForceReason);
  }
  SECTION("two levels of includes inherit the refusal") {
    const std::string deep = dir.write("deep.cfg",
                                       "INIT2B true\n"     // line 1
                                       "RCUT2B 5.0\n"      // line 2
                                       "force true\n");    // line 3
    const std::string mid = dir.write("mid.cfg",
                                      "WATOM 73\n"              // line 1
                                      "CONFIG " + deep + "\n");  // line 2
    const std::string top = dir.write("top.cfg",
                                      "ATOM Ta\n"               // line 1
                                      "CONFIG " + mid + "\n");   // line 2
    const std::string msg =
        runtime_error_of([&] { return parser.parse(top); }, "parse " + top);
    check_refusal(msg, "FORCE", 3, deep, kForceReason);
  }
  SECTION("a refused key after an include names the including file") {
    const std::string inner = dir.write("clean_inner.cfg", "WATOM 73\n");
    const std::string outer = dir.write("after_include.cfg",
                                        "ATOM Ta\n"               // line 1
                                        "CONFIG " + inner + "\n"  // line 2
                                        "INIT2B true\n"           // line 3
                                        "\n"                      // line 4
                                        "FORCE true\n");          // line 5
    const std::string msg = runtime_error_of(
        [&] { return parser.parse(outer); }, "parse " + outer);
    check_refusal(msg, "FORCE", 5, outer, kForceReason);
  }
}

// Pins: refusal is by exact key, never by text. A file whose keys are not
// refused -- with a comment and an inline comment that mention FORCE, a key
// whose name contains FORCE (ZERO_COM_FORCE), FWEIGHT, and a DBFILE value
// FORCE -- parses to exactly the Context a plain parser returns, CONFIG
// include included.
// Before refuse_keys existed this case failed before any parse. Refusing
// by text (any line that mentions FORCE) instead of by key makes it fail.
TEST_CASE("Config parser: refuse_keys leaves a file without refused keys "
          "unchanged",
          "[config][refuse_keys]") {
  RefuseScratch dir;
  const std::string inner = dir.write(
      "plain_inner.cfg",
      "WATOM 73\n"
      "# FORCE true is decided by the configuration file\n"
      "ZERO_COM_FORCE true\n");
  const std::string file = dir.write("plain.cfg",
                                     "ATOM Ta\n"
                                     "INIT2B true   # not FORCE\n"
                                     "RCUT2B 5.0\n"
                                     "FWEIGHT 2.0\n"
                                     "DBFILE FORCE\n"
                                     "CONFIG " + inner + "\n");

  CoreUserConfigParser refusing;
  refusing.refuse_keys({{"FORCE", kForceReason}, {"STRESS", kStressReason}});

  Context with_refusal;
  REQUIRE_NOTHROW(with_refusal = refusing.parse(file));
  const Context plain = CoreUserConfigParser().parse(file);
  CHECK(with_refusal == plain);
  CHECK_FALSE(with_refusal.has_stored("FORCE"));
  CHECK_FALSE(with_refusal.has_stored("STRESS"));
  CHECK(with_refusal.get<bool>("ZERO_COM_FORCE") == true);
  REQUIRE(with_refusal.size("DBFILE") == 1);
  CHECK(with_refusal("DBFILE")[0] == "FORCE");
}

// Pins: refused keys belong to the parser they were given to. The parser
// keeps refusing on every parse; after it refused (also from inside an
// include, which unwinds a nested parse) or parsed a file with an include
// successfully, a new parser -- plain, or refusing another key -- reads
// the same files and stores FORCE.
// Before refuse_keys existed this case failed before any parse. Keeping
// the refused keys in a static or thread_local, or in parse state that an
// exception from inside an include leaves set, makes it fail.
TEST_CASE("Config parser: refuse_keys does not leak into another parser",
          "[config][refuse_keys]") {
  RefuseScratch dir;
  const std::string with_force = dir.write("force.cfg", "ATOM Ta\nFORCE true\n");
  const std::string inc_force = dir.write("inc_force.cfg", "FORCE true\n");
  const std::string outer_force = dir.write(
      "outer_force.cfg", "ATOM Ta\nCONFIG " + inc_force + "\n");
  const std::string inc_clean = dir.write("inc_clean.cfg", "WATOM 73\n");
  const std::string outer_clean = dir.write(
      "outer_clean.cfg", "ATOM Ta\nCONFIG " + inc_clean + "\n");

  {
    CoreUserConfigParser refusing;
    refusing.refuse_keys({{"FORCE", kForceReason}});
    CHECK_THROWS_AS(refusing.parse(with_force), std::runtime_error);
    CHECK_THROWS_AS(refusing.parse(with_force), std::runtime_error);
    CHECK_NOTHROW(refusing.parse(outer_clean));
    CHECK_THROWS_AS(refusing.parse(outer_force), std::runtime_error);
    CHECK_NOTHROW(refusing.parse(outer_clean));
  }

  const CoreUserConfigParser plain{};
  Context direct, included;
  REQUIRE_NOTHROW(direct = plain.parse(with_force));
  CHECK(direct.get<bool>("FORCE") == true);
  REQUIRE_NOTHROW(included = plain.parse(outer_force));
  CHECK(included.get<bool>("FORCE") == true);

  CoreUserConfigParser stress_only;
  stress_only.refuse_keys({{"STRESS", kStressReason}});
  Context other;
  REQUIRE_NOTHROW(other = stress_only.parse(outer_force));
  CHECK(other.get<bool>("FORCE") == true);
}

// Pins: the refusal is an option of the parser, not of file input: the
// stream and line overloads refuse too, naming the line (counted from
// first_line for a block of a file) and, for a block, its source.
// Before refuse_keys existed this case failed before any parse. A stream
// or line overload of parse() that does not pass the refused keys on makes
// it fail.
TEST_CASE("Config parser: refuse_keys applies to stream and line input",
          "[config][refuse_keys]") {
  CoreUserConfigParser parser;
  parser.refuse_keys({{"FORCE", kForceReason}});

  SECTION("a stream") {
    std::istringstream in("ATOM Ta\n\nFORCE true\n");  // FORCE on line 3
    const std::string msg =
        runtime_error_of([&] { return parser.parse(in); }, "parse(stream)");
    check_refusal(msg, "FORCE", 3, "", kForceReason);
  }
  SECTION("lines") {
    const std::vector<std::string> lines = {"ATOM Ta", "force true"};
    const std::string msg =
        runtime_error_of([&] { return parser.parse(lines); }, "parse(lines)");
    check_refusal(msg, "FORCE", 2, "", kForceReason);
  }
  SECTION("a block of lines that starts at line 4 of run.task") {
    /* Lines 4, 5 and 6 of run.task: FORCE is on line 6. */
    const std::vector<std::string> lines = {"ATOM Ta", "# a comment",
                                            "FORCE true"};
    const std::string msg = runtime_error_of(
        [&] { return parser.parse(lines, 4, "run.task"); },
        "parse(lines, 4, run.task)");
    check_refusal(msg, "FORCE", 6, "run.task", kForceReason);
  }
}

// ---------------------------------------------------------------------------
// CONFIG with an inline comment
//
// Every key may carry an inline comment ("KEY value   # comment"); CONFIG,
// whose value is the path of the file to include, must too. The cases use
// the scratch directory of the refuse_keys cases above (RefuseScratch: unique
// to the process, removed with its files).
// ---------------------------------------------------------------------------
namespace {

/* The keys an included file sets, checked after the include. */
void check_included_keys(const Context &c) {
  CHECK(c.get<double>("WATOM") == 73.0);
  CHECK(c.get<double>("RCUT2B") == 5.0);
}

} // namespace

// Pins: a CONFIG line whose path is followed by an inline comment (blanks
// or a tab before the '#') includes the file it names, in a file, a stream
// and the lines of a TASK block (parse(lines, first_line, source), which is
// how `tadah --task` parses each TASK section): the included keys are set,
// and the result equals that of the same line without the comment.
// Before the fix: every section threw std::runtime_error "Failed to open
// config file: <path>   " -- the path was read up to the '#', with the blanks
// before it, so no such file existed (`tadah --task` and `tadah train -c`
// stopped with that error).
TEST_CASE("Config parser: a CONFIG line with an inline comment includes the "
          "file it names",
          "[config][config_include]") {
  RefuseScratch dir;
  const std::string inner =
      dir.write("inner_settings.cfg", "WATOM 73\nRCUT2B 5.0\n");
  const std::string plain = dir.write(
      "plain_include.cfg", "ATOM Ta\nCONFIG " + inner + "\nINIT2B true\n");
  const Context expected = CoreUserConfigParser().parse(plain);
  check_included_keys(expected);

  SECTION("a file, blanks before the comment") {
    const std::string outer =
        dir.write("commented_include.cfg",
                  "ATOM Ta\n"
                  "CONFIG " + inner + "   # nested config\n"
                  "INIT2B true\n");
    Context c;
    REQUIRE_NOTHROW(c = CoreUserConfigParser().parse(outer));
    check_included_keys(c);
    CHECK(c == expected);
  }
  SECTION("a file, a tab before the comment") {
    const std::string outer =
        dir.write("tab_include.cfg",
                  "ATOM Ta\n"
                  "CONFIG " + inner + "\t# nested config\n"
                  "INIT2B true\n");
    Context c;
    REQUIRE_NOTHROW(c = CoreUserConfigParser().parse(outer));
    CHECK(c == expected);
  }
  SECTION("a stream") {
    std::istringstream in("ATOM Ta\nCONFIG " + inner +
                          "   # nested config\nINIT2B true\n");
    Context c;
    REQUIRE_NOTHROW(c = CoreUserConfigParser().parse(in));
    CHECK(c == expected);
  }
  SECTION("the lines of a TASK block (lines 4 to 6 of run.task)") {
    const std::vector<std::string> lines = {
        "ATOM Ta", "CONFIG " + inner + "   # nested config",
        "INIT2B true   # two-body"};
    Context c;
    REQUIRE_NOTHROW(c = CoreUserConfigParser().parse(lines, 4, "run.task"));
    CHECK(c == expected);
  }
}

// Pins: with an inline comment on the CONFIG line, a missing file is reported
// by its path alone ("Failed to open config file: <path>", nothing after the
// path), and a circular include names the files of the chain, each by its
// path alone ("... CONFIG files: <b> -> <a> -> <b>").
// Before the fix: the missing file was reported as "<path>   " (the blanks
// before the '#'), and the circular include was never detected: the first
// CONFIG line already failed to open "<b>  ", so the message was "Failed to
// open config file: <b>  " instead of the chain.
TEST_CASE("Config parser: a CONFIG line with an inline comment names the "
          "file alone in its errors",
          "[config][config_include]") {
  RefuseScratch dir;

  SECTION("a missing file") {
    const std::string missing = dir.path("missing.cfg");
    const std::string outer = dir.write(
        "missing_include.cfg", "CONFIG " + missing + "   # not there\n");
    const std::string msg = runtime_error_of(
        [&] { return CoreUserConfigParser().parse(outer); }, "parse " + outer);
    CHECK(msg == "Failed to open config file: " + missing);
  }
  SECTION("a circular include") {
    const std::string b = dir.write(
        "loop_b.cfg",
        "WATOM 73\nCONFIG " + dir.path("loop_a.cfg") + "  # back to a\n");
    const std::string a =
        dir.write("loop_a.cfg", "ATOM Ta\nCONFIG " + b + "  # on to b\n");
    const std::string msg = runtime_error_of(
        [&] { return CoreUserConfigParser().parse(a); }, "parse " + a);
    CHECK(msg == "Circular dependency detected between CONFIG files: " + b +
                     " -> " + a + " -> " + b);
  }
}

namespace {

/* Captures what the Logger writes (to std::cout) with ERROR lines on;
 * restores the stream and the level afterwards. */
class CaptureLog {
public:
  CaptureLog()
      : level_(Logger::instance().getLogLevel()),
        old_(std::cout.rdbuf(out_.rdbuf())) {
    Logger::instance().setLogLevel(LogLevel::ERROR);
  }
  ~CaptureLog() {
    std::cout.rdbuf(old_);
    Logger::instance().setLogLevel(level_);
  }
  std::string text() const { return out_.str(); }

private:
  LogLevel level_;
  std::ostringstream out_;
  std::streambuf *old_;
};

/* The text of each line of `log`, after the Logger's "[time] ERROR: "
 * prefix (a line without one, such as the rest of a message that holds a
 * newline, as it is). */
std::vector<std::string> log_lines(const std::string &log) {
  std::vector<std::string> out;
  std::istringstream in(log);
  std::string line;
  const std::string tag = "ERROR: ";
  while (std::getline(in, line)) {
    const auto at = line.find(tag);
    out.push_back(at == std::string::npos ? line : line.substr(at + tag.size()));
  }
  return out;
}

std::string upper_case(std::string t) {
  for (char &ch : t)
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  return t;
}

std::string trimmed(const std::string &t) {
  const auto b = t.find_first_not_of(" \t\r");
  if (b == std::string::npos) return "";
  return t.substr(b, t.find_last_not_of(" \t\r") - b + 1);
}

} // namespace

/* Pins: when CoreUserConfigParser refuses a value of a key (the value does
 * not have the key's format), the lines it writes under "  Examples:" are,
 * for every key whose value can be refused (one of "@", "", "1.5", "@ @ @"
 * is), exactly one line "    <KEY>   <line>" for each non-blank line of each
 * of the key's examples, in order, and the word after the key is never the
 * key again.
 * Before the fix: an example that began with its key was printed with the
 * key twice ("    BETA   BETA 0.0001"), and an example of several lines was
 * printed as one message, whose later lines were the example's own text
 * without the "    <KEY>   " in front (or the log prefix of a line):
 * "    CGRID2B   CGRID2B   1.0 2.0" then "CGRID2B   1.5 2.5", and
 * "  TYPE2B    D2_MIE 11 6 Ti Ti" after "    TYPE2B   TYPE2B    D2_mJoin". */
TEST_CASE("Config parser: a refused value lists each example once, after "
          "its key", "[config][examples]") {
  std::size_t keys = 0;
  for (const auto &kv : BlueprintParser::getKeysMapRef()) {
    const std::string &key = kv.first;
    const BlueprintKey &k = kv.second;
    if (k.examples.empty()) continue;
    std::string refused;
    bool found = false;
    for (const std::string candidate : {"@", "", "1.5", "@ @ @"}) {
      if (!CoreUserConfigParser::validateSchemaOnly(candidate, k)) {
        refused = candidate;
        found = true;
        break;
      }
    }
    if (!found) continue;   /* every value has its format: never refused */
    ++keys;
    INFO("key " << key << ", refused value '" << refused << "'");

    std::vector<std::string> expected;
    for (const std::string &example : k.examples) {
      std::istringstream lines(example);
      std::string line;
      while (std::getline(lines, line))
        if (!trimmed(line).empty())
          expected.push_back("    " + key + "   " + trimmed(line));
    }

    std::string log;
    {
      CaptureLog capture;
      CHECK_FALSE(CoreUserConfigParser::validateWithBlueprint(key, refused, k));
      log = capture.text();
    }
    const std::vector<std::string> all = log_lines(log);
    const auto head = std::find(all.begin(), all.end(), "  Examples:");
    REQUIRE(head != all.end());
    const std::vector<std::string> printed(head + 1, all.end());
    CHECK(printed == expected);
    for (const std::string &line : printed) {
      INFO("printed '" << line << "'");
      std::istringstream words(line);
      std::string first, second;
      words >> first >> second;
      CHECK(upper_case(first) == key);
      CHECK(upper_case(second) != key);
    }
  }
  CHECK(keys > 100);
}

/* Pins: a key that CoreUserConfigParser reads as a block -- the key alone
 * on its line opens it and the lines up to END<KEY> are its value: the
 * `tadah refit` stages INIT, EXPLORE, REFINE and FUSE and their nested
 * OPTIMIZER -- has no examples in the blueprint. "<KEY>   <value>", as the
 * parser's refusal and the configuration-keys page show an example, is not
 * how such a key is written (the parser does not read the rest of the
 * opening line). The block keys are found from the parser itself: the key
 * alone on a line is refused as a block that is not closed.
 * Before the fix: all five had an example, "INIT / PHI XMIN PIN / ENDINIT"
 * and the like, and OPTIMIZER's was "see HPO_REFIT_2B_EAM/REDESIGN/configs
 * (OPTIMIZER nested in EXPLORE/REFINE/FUSE)", a directory that is in no
 * Tadah repository. */
TEST_CASE("Config parser: a key written as a block has no examples",
          "[config][examples]") {
  std::set<std::string> blocks;
  for (const auto &kv : BlueprintParser::getKeysMapRef()) {
    const std::string &key = kv.first;
    std::string what;
    {
      CaptureLog quiet;
      try {
        CoreUserConfigParser().parse(std::vector<std::string>{key});
      } catch (const std::exception &e) {
        what = e.what();
      }
    }
    if (what != key + " block is not closed with END" + key) continue;
    blocks.insert(key);
    INFO("block key " << key);
    CHECK(kv.second.examples.empty());
  }
  CHECK(blocks == std::set<std::string>{"EXPLORE", "FUSE", "INIT",
                                        "OPTIMIZER", "REFINE"});
}

namespace {
/* The message parse() throws for `lines` (first line `first` of `source`),
 * or "no exception". */
std::string parse_refusal(const std::vector<std::string> &lines,
                          std::size_t first = 1,
                          const std::string &source = "") {
  try {
    (void)CoreUserConfigParser().parse(lines, first, source);
  } catch (const std::exception &e) {
    return e.what();
  }
  return "no exception";
}
}  // namespace

/* Pins: a configuration line whose list values are joined by a comma or a
 * '+' (which the parser does not split at: it splits a line at blanks) is
 * refused when it is parsed, naming the line, the key, the value and the
 * separator, and the line to write instead:
 *   "Invalid value for key FIXWEIGHT on line 1: '0.1,0.2' separates values
 *    with ',', which a configuration line is not split at (values are
 *    separated by blanks: FIXWEIGHT 0.1 0.2)"
 * also with the file and line of a task section (line 3 of run.task), for
 * '+' ('MPARAMS 1.0+2.0'), for both ('CGRID2B 1.0,2.0+3.0': "',' and
 * '+'"), with blanks around the comma ('SGRID2B 1.0, 2.0': SGRID2B 1.0
 * 2.0), for a key of several single values ('MODEL M_BLR,BF_Linear'), for an
 * index list joined by '+' ('FIXINDEX 13+14'), and for a line of 2048
 * characters or more. A value that blanks would not make valid either keeps
 * the generic refusal ('SBASIS 1,2': "Invalid schema for key: SBASIS").
 * What the parser keeps as one value is accepted: an index list with commas
 * ('FIXINDEX 13,14', its reader splits it), a STRING word with a comma or a
 * '+' ('DBFILE a,b.tadah', 'ATOM Ti+Nb'), an exponent's sign
 * ('FIXWEIGHT 1e+5 2').
 * Before the fix: 'FIXWEIGHT 0.1,0.2', 'MPARAMS 1.0+2.0', 'CGRID2B
 * 1.0,2.0+3.0', 'SGRID2B 1.0, 2.0', 'FIXINDEX 13+14' and the long line were
 * accepted (one value per word, '0.1,0.2' refused only when read, and
 * '13+14' by FIXINDEX's reader), and 'MODEL M_BLR,BF_Linear' was refused as
 * "Invalid schema for key: MODEL". */
TEST_CASE("Config parser: a comma or '+' between values is refused, naming "
          "the line, the key and the separator",
          "[config][separators]") {
  const std::string tail = ", which a configuration line is not split at "
                           "(values are separated by blanks: ";
  struct Case {
    std::string line, message;
  };
  const std::vector<Case> cases = {
      {"FIXWEIGHT 0.1,0.2",
       "Invalid value for key FIXWEIGHT on line 1: '0.1,0.2' separates "
       "values with ','" + tail + "FIXWEIGHT 0.1 0.2)"},
      {"MPARAMS 1.0+2.0",
       "Invalid value for key MPARAMS on line 1: '1.0+2.0' separates values "
       "with '+'" + tail + "MPARAMS 1.0 2.0)"},
      {"CGRID2B 1.0,2.0+3.0",
       "Invalid value for key CGRID2B on line 1: '1.0,2.0+3.0' separates "
       "values with ',' and '+'" + tail + "CGRID2B 1.0 2.0 3.0)"},
      {"SGRID2B 1.0, 2.0",
       "Invalid value for key SGRID2B on line 1: '1.0, 2.0' separates "
       "values with ','" + tail + "SGRID2B 1.0 2.0)"},
      {"model M_BLR,BF_Linear",
       "Invalid value for key MODEL on line 1: 'M_BLR,BF_Linear' separates "
       "values with ','" + tail + "MODEL M_BLR BF_Linear)"},
      {"FIXINDEX 13+14",
       "Invalid value for key FIXINDEX on line 1: '13+14' separates values "
       "with '+'" + tail + "FIXINDEX 13 14)"},
      {"SBASIS 1,2", "Invalid schema for key: SBASIS"},
  };
  for (const Case &c : cases) {
    INFO(c.line);
    CHECK(parse_refusal({c.line}) == c.message);
  }
  // The file and line of a task section.
  CHECK(parse_refusal({"# comment", "VERBOSE 1", "FIXWEIGHT 0.1,0.2"}, 1,
                      "run.task") ==
        "Invalid value for key FIXWEIGHT on line 3 of run.task: '0.1,0.2' "
        "separates values with ','" + tail + "FIXWEIGHT 0.1 0.2)");
  // A line of 2048 characters or more.
  std::string long_line = "FIXWEIGHT 1.0";
  while (long_line.size() < 2100) long_line += ",2.0";
  const std::string long_msg = parse_refusal({long_line});
  INFO(long_msg.substr(0, 80));
  CHECK(long_msg.rfind("Invalid value for key FIXWEIGHT on line 1: "
                       "'1.0,2.0,2.0,",
                       0) == 0);
  CHECK(long_msg.find("' separates values with ','" + tail +
                      "FIXWEIGHT 1.0 2.0 2.0 ") != std::string::npos);

  // One value per word, as the parser keeps it.
  Context c;
  REQUIRE_NOTHROW(c = CoreUserConfigParser().parse(std::vector<std::string>{
                      "FIXINDEX 13,14", "DBFILE a,b.tadah", "ATOM Ti+Nb",
                      "FIXWEIGHT 1e+5 2"}));
  CHECK(c.size("FIXINDEX") == 1);
  CHECK(c.get<std::string>("FIXINDEX") == "13,14");
  CHECK(c.get<std::string>("DBFILE") == "a,b.tadah");
  CHECK(c.get<std::string>("ATOM") == "Ti+Nb");
  CHECK(c.size("FIXWEIGHT") == 2);
  CHECK(c.get<double>("FIXWEIGHT") == 1e5);
}
