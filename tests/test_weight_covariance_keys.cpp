// The keys of a potential's weight covariance (blueprint): WCOV, the
// covariance of the WEIGHTS (N, then the N x N matrix column-major), and
// WNOISE, the noise variance of each row type the fit had (E, F, S), both
// written by an OALGO 3 fit and read by `tadah predict --error`; and SIGMA,
// the matrix Tadah! 1.0.0 to 1.5.0 and the 1.6.0 development builds wrote
// instead, now a deprecated key: accepted so that those potentials still
// load (predict, train, pair_style tadah), with one notice, and never
// stored.
//
// ctest runs every test case in a process of its own, where no deprecated
// key has been reported yet; a case counts a notice only when
// core::ignored_key_reported() says the key was not reported before it.

#include "catch2/catch.hpp"

#include <tadah/core/context.h>
#include <tadah/core/core_user_config_parser.h>
#include <tadah/core/key_notice.h>
#include <tadah/core/logger.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace tadah::core;

namespace {

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

}  // namespace

/* Pins: a potential with a SIGMA line (as Tadah! 1.0.0 to 1.5.0 and the
 * 1.6.0 development builds wrote for an OALGO 3 fit, here "SIGMA 2 1 0 0
 * 1") parses: one WARNING on standard error, "Deprecated key SIGMA is
 * ignored", naming the transposed formula,
 * WCOV and OALGO 3; the key is not stored (has_stored false) and a
 * potential written from the Context has no SIGMA line; the other keys are
 * kept.
 * Before the fix: SIGMA was stored, with no notice (and `tadah predict
 * --error` read it as the covariance of the weights). */
TEST_CASE("Config parser: SIGMA parses, warns once and is not stored",
          "[config][deprecated][weight_covariance]") {
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
    CHECK(warned[0].find("WCOV") != std::string::npos);
    CHECK(warned[0].find("OALGO 3") != std::string::npos);
  }
  CHECK_FALSE(c.has_stored("SIGMA"));
  CHECK(c.get<std::string>("ATOM") == "Ta");
  CHECK(key_line(serialized(c), "SIGMA").empty());
}

/* Pins: WCOV (N, then N x N values) and WNOISE (row type and noise
 * variance pairs) parse and are stored as written: "WCOV 2 0.5 0.1 0.1
 * 0.25" (5 values, N = 2) and "WNOISE E 0.04 F 0.0025 S 1e-6" (6 tokens),
 * and the Context written with operator<< has both lines with the same
 * tokens.
 * Before the fix: "Invalid key: WCOV" (and WNOISE): no potential with them
 * loaded. */
TEST_CASE("Config parser: WCOV and WNOISE parse and are stored",
          "[config][weight_covariance]") {
  Context c;
  REQUIRE_NOTHROW(c = CoreUserConfigParser().parse(std::vector<std::string>{
                      "ATOM Ta", "WCOV 2 0.5 0.1 0.1 0.25",
                      "WNOISE E 0.04 F 0.0025 S 1e-6"}));
  REQUIRE(c.size("WCOV") == 5);
  CHECK(c.get<std::size_t>("WCOV") == 2);
  CHECK(c.get<double>("WCOV", 4) == 0.25);
  REQUIRE(c.size("WNOISE") == 6);
  CHECK(c.get<std::string>("WNOISE", 0) == "E");
  CHECK(c.get<double>("WNOISE", 1) == 0.04);
  CHECK(c.get<std::string>("WNOISE", 4) == "S");
  const std::string pot = serialized(c);
  INFO(pot);
  CHECK(key_line(pot, "WCOV") ==
        std::vector<std::string>{"2", "0.5", "0.1", "0.1", "0.25"});
  CHECK(key_line(pot, "WNOISE") ==
        std::vector<std::string>{"E", "0.04", "F", "0.0025", "S", "1e-6"});
}

/* Pins: a WNOISE that is not row type and number pairs ("WNOISE E", "WNOISE
 * E 0.1 F") and a WCOV whose first value is not an integer ("WCOV 1.5 1")
 * are refused at parse with the parser's schema error naming the key
 * ("Invalid schema for key: WNOISE ...").
 * Before the fix: "Invalid key: WNOISE" (the key did not exist). */
TEST_CASE("Config parser: a malformed WNOISE or WCOV is refused",
          "[config][weight_covariance]") {
  for (const std::string line :
       {"WNOISE E", "WNOISE E 0.1 F", "WCOV 1.5 1"}) {
    INFO(line);
    const std::string msg = refusal({"ATOM Ta", line});
    INFO("message: " << msg);
    CHECK(msg.find("Invalid schema for key") != std::string::npos);
    CHECK(msg.find(line.substr(0, line.find(' '))) != std::string::npos);
  }
}
