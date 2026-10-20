// The keys and options of uncertainty output, which Tadah! 1.6.0 does not
// give: its error bars were not calibrated (they were too narrow for many
// structures).
//
//  - WCOV and WNOISE (the covariance of the WEIGHTS and the noise variance
//    of each quantity of an OALGO 3 fit) and UNCERTAINTY (tadah train
//    --uncertainty; tadah train 1.5.0 wrote an UNCERTAINTY line, true or
//    false, into every potential) are retired keys: a file with them parses,
//    with one notice per parse, the lines are recorded and never stored,
//    and a fit refuses them (retired_keys.h).
//  - --error is a retired option of tadah predict and of tadah properties
//    pairwise (the blueprint field `retired` of the task's key): those
//    tasks refuse it.
//  - SIGMA, the matrix Tadah! 1.0.0 to 1.5.0 wrote for an OALGO 3 fit,
//    stays a deprecated key: accepted so that those potentials still load,
//    with one notice, and never stored.
//
// ctest runs every test case in a process of its own, where no deprecated
// key has been reported yet; a case counts a notice only when
// core::ignored_key_reported() says the key was not reported before it.

#include "catch2/catch.hpp"

#include <tadah/core/blueprint_parser.h>
#include <tadah/core/context.h>
#include <tadah/core/core_user_config_parser.h>
#include <tadah/core/key_notice.h>
#include <tadah/core/logger.h>
#include <tadah/core/retired_keys.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace tadah::core;
using Catch::Matchers::Contains;
using Catch::Matchers::StartsWith;

namespace {

/* The sentence every retired key and option of uncertainty output gives as
 * the reason it is retired. */
const std::string kNotPart =
    "Uncertainty output is not part of Tadah! 1.6.0: its error bars were "
    "not calibrated (they were too narrow for many structures).";

/* Captures std::cerr (where the deprecation notice goes) and std::cout
 * (the Logger) at WARNING level; restores them afterwards. */
class StreamCapture {
public:
  StreamCapture()
      : old_level_(Logger::instance().getLogLevel()),
        old_out_(std::cout.rdbuf(out_.rdbuf())),
        old_err_(std::cerr.rdbuf(err_.rdbuf())) {
    Logger::instance().setLogLevel(LogLevel::WARNING);
  }
  ~StreamCapture() {
    std::cout.rdbuf(old_out_);
    std::cerr.rdbuf(old_err_);
    Logger::instance().setLogLevel(old_level_);
  }
  StreamCapture(const StreamCapture &) = delete;
  StreamCapture &operator=(const StreamCapture &) = delete;

  std::vector<std::string> err_lines_with(const std::string &needle) const {
    std::vector<std::string> out;
    std::istringstream in(err_.str());
    for (std::string line; std::getline(in, line);)
      if (line.find(needle) != std::string::npos) out.push_back(line);
    return out;
  }

private:
  std::ostringstream out_;
  std::ostringstream err_;
  LogLevel old_level_;
  std::streambuf *old_out_;
  std::streambuf *old_err_;
};

std::string serialized(const Context &c) {
  std::ostringstream os;
  os << c;
  return os.str();
}

std::vector<std::string> key_line(const std::string &text,
                                  const std::string &key) {
  std::istringstream in(text);
  for (std::string line; std::getline(in, line);) {
    std::istringstream ls(line);
    std::string first;
    if ((ls >> first) && first == key) {
      std::vector<std::string> t;
      for (std::string w; ls >> w;) t.push_back(w);
      return t;
    }
  }
  return {};
}

std::string refusal(const std::vector<std::string> &lines) {
  try {
    CoreUserConfigParser().parse(lines);
  } catch (const std::exception &e) {
    return e.what();
  }
  return "<no exception>";
}

std::vector<std::string> keys_of(const std::vector<RetiredKeyLine> &lines) {
  std::vector<std::string> out;
  for (const auto &l : lines) out.push_back(l.key);
  return out;
}

/* The option @p name ("error") of the blueprint task @p task ("PREDICT"),
 * or nullptr. */
const BlueprintKey *task_option(const std::string &task,
                                const std::string &name) {
  const auto &t = BlueprintParser::getTasksMapRef().at(task);
  const std::string full = task + "." + name;
  for (const BlueprintKey &k : t.blueprintKeys)
    if (k.keyName == full) return &k;
  for (const auto &g : t.groups)
    for (const BlueprintKey &k : g.blueprintKeys)
      if (k.keyName == full) return &k;
  return nullptr;
}

/* The checks of a retired key or option of uncertainty output: why (the
 * output it gave, and kNotPart), that nothing replaces it, its conversion
 * (deleting it; the fit, and the predictions, unaffected), and no default
 * or example. */
void check_retired_output(const BlueprintKey &k, const std::string &what) {
  INFO(k.keyName);
  REQUIRE(k.isRetired());
  CHECK_FALSE(k.isDeprecated());
  CHECK_THAT(k.retired.reason, Contains(what) && Contains(kNotPart));
  CHECK(k.retired.replacement == "nothing in Tadah! 1.6.0");
  CHECK_THAT(k.retired.conversion, StartsWith("Deleting the") &&
                                       Contains("unaffected"));
  CHECK(k.default_value.empty());
  CHECK(k.examples.empty());
}

}  // namespace

/* Pins: WCOV, WNOISE and UNCERTAINTY are retired keys, and --error a
 * retired option of tadah predict and of tadah properties pairwise
 * (PREDICT.error, PROPERTIES.pairwise.error): each says the output it gave
 * and that uncertainty output is not part of Tadah! 1.6.0, its error bars
 * not calibrated (too narrow for many structures); nothing replaces it, and
 * deleting it converts it, the fit (and the predictions) unaffected; none
 * has a default or an example. The command-line options stay, to give the
 * reason: --uncertainty of UNCERTAINTY, --error of the task options. The
 * key ERROR the options inherit is not retired itself (the tasks retire
 * it) and has no default.
 * Before the fix: none was retired; WCOV and WNOISE were written by an
 * OALGO 3 fit and read by tadah predict --error. */
TEST_CASE("Blueprint: uncertainty output is retired: WCOV, WNOISE, "
          "UNCERTAINTY and --error of predict and properties pairwise",
          "[BlueprintParser][retired][uncertainty]") {
  const auto &keys = BlueprintParser::getKeysMapRef();
  check_retired_output(keys.at("WCOV"), "covariance of the WEIGHTS");
  check_retired_output(keys.at("WNOISE"), "noise variance");
  check_retired_output(keys.at("UNCERTAINTY"), "tadah train --uncertainty");
  CHECK(keys.at("UNCERTAINTY").cli_long_flag == "--uncertainty");

  const BlueprintKey *predict = task_option("PREDICT", "error");
  REQUIRE(predict != nullptr);
  check_retired_output(*predict, "tadah predict --error");
  CHECK_THAT(predict->retired.conversion, Contains("predictions"));
  CHECK(predict->cli_long_flag == "--error");

  const BlueprintKey *pairwise = task_option("PROPERTIES.pairwise", "error");
  REQUIRE(pairwise != nullptr);
  check_retired_output(*pairwise, "tadah properties pairwise --error");
  CHECK(pairwise->cli_long_flag == "--error");

  const BlueprintKey *train = task_option("TRAIN", "uncertainty");
  REQUIRE(train != nullptr);
  CHECK(train->isRetired());

  CHECK_FALSE(keys.at("ERROR").isRetired());
  CHECK(keys.at("ERROR").default_value.empty());
}

/* Pins: a potential with WCOV (N = 2, then the 2 x 2 matrix) and WNOISE
 * (row type and noise variance pairs), as a development build of Tadah!
 * 1.6.0 wrote for an OALGO 3 fit, parses: both lines are recorded
 * (Context::retired_keys(), with their values) and neither is stored, a
 * Context written with operator<< has neither line, and the parse gives
 * one notice, "Retired keys WCOV (line 2) and WNOISE (line 3) are
 * ignored: they change nothing a prediction gives, and a fit refuses them
 * (tadah explain WCOV and tadah explain WNOISE give the reasons and the
 * conversions)."
 * Before the fix: both keys were stored as written, with no notice. */
TEST_CASE("Config parser: WCOV and WNOISE are retired: recorded, not "
          "stored, one notice",
          "[config][retired][uncertainty]") {
  IgnoredKeyNoticeCapture cap;
  Context c;
  REQUIRE_NOTHROW(c = CoreUserConfigParser().parse(std::vector<std::string>{
                      "ATOM Ta", "WCOV 2 0.5 0.1 0.1 0.25",
                      "WNOISE E 0.04 F 0.0025 S 1e-6"}));
  CHECK_FALSE(c.has_stored("WCOV"));
  CHECK_FALSE(c.has_stored("WNOISE"));
  CHECK(c.get<std::string>("ATOM") == "Ta");
  const std::vector<RetiredKeyLine> r = c.retired_keys();
  REQUIRE(keys_of(r) == (std::vector<std::string>{"WCOV", "WNOISE"}));
  CHECK(r[0].line == 2);
  CHECK(r[0].values ==
        (std::vector<std::string>{"2", "0.5", "0.1", "0.1", "0.25"}));
  CHECK(r[1].values == (std::vector<std::string>{"E", "0.04", "F", "0.0025",
                                                 "S", "1e-6"}));
  const std::string pot = serialized(c);
  INFO(pot);
  CHECK(key_line(pot, "WCOV").empty());
  CHECK(key_line(pot, "WNOISE").empty());
  REQUIRE(cap.notices().size() == 1);
  CHECK(cap.notices()[0] ==
        "Retired keys WCOV (line 2) and WNOISE (line 3) are ignored: they "
        "change nothing a prediction gives, and a fit refuses them (tadah "
        "explain WCOV and tadah explain WNOISE give the reasons and the "
        "conversions).");
}

/* Pins: an UNCERTAINTY line (tadah train 1.5.0 wrote one, true or false,
 * into every potential) parses as a retired key:
 * recorded, not stored, one notice "Retired key UNCERTAINTY (line 2) is
 * ignored: it changes nothing a prediction gives, and a fit refuses it
 * (tadah explain UNCERTAINTY gives the reason and the conversion)."; a
 * notice of retired keys of several kinds says no more than that (not
 * that they weighted the training data).
 * Before the fix: UNCERTAINTY was stored, with no notice. */
TEST_CASE("Config parser: UNCERTAINTY is a retired key",
          "[config][retired][uncertainty]") {
  IgnoredKeyNoticeCapture cap;
  Context c;
  REQUIRE_NOTHROW(c = CoreUserConfigParser().parse(std::vector<std::string>{
                      "ATOM Ta", "UNCERTAINTY true"}));
  CHECK_FALSE(c.has_stored("UNCERTAINTY"));
  REQUIRE(keys_of(c.retired_keys()) ==
          std::vector<std::string>{"UNCERTAINTY"});
  REQUIRE(cap.notices().size() == 1);
  CHECK(cap.notices()[0] ==
        "Retired key UNCERTAINTY (line 2) is ignored: it changes nothing a "
        "prediction gives, and a fit refuses it (tadah explain UNCERTAINTY "
        "gives the reason and the conversion).");

  CoreUserConfigParser().parse(std::vector<std::string>{
      "EWEIGHT 1", "UNCERTAINTY true", "WCOV 1 0.5"});
  REQUIRE(cap.notices().size() == 2);
  CHECK_THAT(cap.notices()[1],
             StartsWith("Retired keys EWEIGHT (line 1), UNCERTAINTY (line "
                        "2) and WCOV (line 3) are ignored: they change "
                        "nothing a prediction gives"));
  CHECK(cap.notices()[1].find("training data") == std::string::npos);
}

/* Pins: Context::add() refuses WCOV, WNOISE and UNCERTAINTY with the
 * reason ("<KEY> is a retired key: ... Uncertainty output is not part of
 * Tadah! 1.6.0 ..."), and stores nothing.
 * Before the fix: each was stored. */
TEST_CASE("Context::add refuses WCOV, WNOISE and UNCERTAINTY with the reason",
          "[context][retired][uncertainty]") {
  Context c;
  REQUIRE_THROWS_WITH(c.add("WCOV", "2"),
                      Contains("WCOV is a retired key") && Contains(kNotPart));
  REQUIRE_THROWS_WITH(c.add("wnoise", "E"),
                      Contains("WNOISE is a retired key") && Contains(kNotPart));
  REQUIRE_THROWS_WITH(c.add("UNCERTAINTY", true),
                      Contains("UNCERTAINTY is a retired key") &&
                          Contains(kNotPart));
  CHECK_FALSE(c.has_stored("WCOV"));
  CHECK_FALSE(c.has_stored("WNOISE"));
  CHECK_FALSE(c.has_stored("UNCERTAINTY"));
}

/* Pins: a fit refuses WCOV and WNOISE with the conversion of each
 * (retired_keys_refusal), and names a line of more than six values by its
 * first three and the number of the others: "WCOV 3 1 0 and 7 more values
 * (line 2)" for the ten values of N = 3; a line of six values or fewer is
 * named whole ("WNOISE E 0.04 F 0.0025 S 1e-6 (line 3)").
 * Before the fix: neither was retired, so a fit read none (and a line of
 * a retired key was named with all its values). */
TEST_CASE("A fit refuses WCOV and WNOISE, naming a long line by its first "
          "values",
          "[retired][uncertainty]") {
  IgnoredKeyNoticeCapture quiet;
  const Context c = CoreUserConfigParser().parse(std::vector<std::string>{
      "ATOM Ta", "WCOV 3 1 0 0 0 1 0 0 0 1", "WNOISE E 0.04 F 0.0025 S 1e-6"});
  const std::string msg = retired_keys_refusal(c.retired_keys(), "tadah train");
  INFO(msg);
  CHECK_THAT(msg, StartsWith("tadah train: "));
  CHECK_THAT(msg, Contains("WCOV 3 1 0 and 7 more values (line 2)") &&
                      Contains("WNOISE E 0.04 F 0.0025 S 1e-6 (line 3)"));
  const auto &keys = BlueprintParser::getKeysMapRef();
  CHECK_THAT(msg, Contains(keys.at("WCOV").retired.conversion) &&
                      Contains(keys.at("WNOISE").retired.conversion));
  REQUIRE_THROWS_WITH(require_no_retired_keys(c, "A fit"),
                      StartsWith("A fit: ") && Contains("WCOV 3 1 0"));
}

/* Pins: a potential with a SIGMA line (as Tadah! 1.0.0 to 1.5.0 wrote for
 * an OALGO 3 fit, here "SIGMA 2 1 0 0 1") parses: one WARNING on standard
 * error, "Deprecated key SIGMA is ignored", naming the transposed formula
 * and saying that uncertainty output is not part of Tadah! 1.6.0, without
 * pointing to WCOV or a retrain; the key is not stored (has_stored false)
 * and a potential written from the Context has no SIGMA line; the other
 * keys are kept.
 * Before the fix: the notice said that a retrain with OALGO 3 writes WCOV
 * and WNOISE, which tadah predict --error needs. */
TEST_CASE("Config parser: SIGMA parses, warns once and is not stored",
          "[config][deprecated][uncertainty]") {
  const std::size_t expected = ignored_key_reported("SIGMA") ? 0 : 1;
  StreamCapture cap;
  Context c;
  REQUIRE_NOTHROW(c = CoreUserConfigParser().parse(std::vector<std::string>{
                      "ATOM Ta", "SIGMA 2 1 0 0 1",
                      "SIGMA 2 1 0 0 1   # a second line"}));
  const auto warned = cap.err_lines_with("Deprecated key SIGMA is ignored");
  REQUIRE(warned.size() == expected);
  if (!warned.empty()) {
    INFO(warned[0]);
    CHECK(warned[0].find("transposed") != std::string::npos);
    CHECK(warned[0].find(kNotPart) != std::string::npos);
    CHECK(warned[0].find("WCOV") == std::string::npos);
    CHECK(warned[0].find("retrain") == std::string::npos);
  }
  CHECK_FALSE(c.has_stored("SIGMA"));
  CHECK(c.get<std::string>("ATOM") == "Ta");
  CHECK(key_line(serialized(c), "SIGMA").empty());
}

/* Pins: a WNOISE that is not row type and number pairs ("WNOISE E", "WNOISE
 * E 0.1 F") and a WCOV whose first value is not an integer ("WCOV 1.5 1")
 * are refused at parse with the parser's schema error naming the key
 * ("Invalid schema for key: WNOISE ..."): the value of a retired key is
 * still checked.
 * Before the fix: refused alike (a pin: the keys kept their types). */
TEST_CASE("Config parser: a malformed WNOISE or WCOV is refused",
          "[config][uncertainty]") {
  for (const std::string line :
       {"WNOISE E", "WNOISE E 0.1 F", "WCOV 1.5 1"}) {
    INFO(line);
    const std::string msg = refusal({"ATOM Ta", line});
    INFO("message: " << msg);
    CHECK(msg.find("Invalid schema for key") != std::string::npos);
    CHECK(msg.find(line.substr(0, line.find(' '))) != std::string::npos);
  }
}
