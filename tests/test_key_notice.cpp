// The notice for a key that is accepted but has no effect (key_notice.h):
// one line on standard error, whatever the Logger level, and at most once
// per process for report_ignored_key_once().

#include "catch2/catch.hpp"

#include <tadah/core/key_notice.h>
#include <tadah/core/logger.h>

#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace tadah::core;

namespace {

/* Captures std::cerr and std::cout and sets the Logger level; restores the
 * streams and the level afterwards. */
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

  std::string err() const { return err_.str(); }
  std::string out() const { return out_.str(); }

private:
  std::ostringstream out_;
  std::ostringstream err_;
  LogLevel old_level_;
  std::streambuf *old_out_;
  std::streambuf *old_err_;
};

std::size_t count_of(const std::string &text, const std::string &needle) {
  std::size_t n = 0;
  for (std::size_t p = text.find(needle); p != std::string::npos;
       p = text.find(needle, p + needle.size()))
    ++n;
  return n;
}

} // namespace

TEST_CASE("report_ignored_key: one WARNING line on stderr, whatever the "
          "Logger level",
          "[key_notice]") {
  for (const LogLevel level : {LogLevel::ERROR, LogLevel::WARNING}) {
    StreamCapture cap(level);
    report_ignored_key("Key TEST_NOTICE_A", "the reason.");
    CHECK(cap.err() == "WARNING: Key TEST_NOTICE_A is ignored: the reason.\n");
    CHECK(cap.out().empty());
  }
}

TEST_CASE("report_ignored_key_once: once per key and process, also across "
          "threads",
          "[key_notice]") {
  const std::string key = "TEST_NOTICE_ONCE_KEY";
  const std::string line = "WARNING: Key " + key + " is ignored: why.\n";
  REQUIRE_FALSE(ignored_key_reported(key));
  StreamCapture cap(LogLevel::ERROR);
  CHECK(report_ignored_key_once(key, "Key " + key, "why."));
  CHECK(ignored_key_reported(key));
  CHECK_FALSE(report_ignored_key_once(key, "Key " + key, "why."));
  // another subject or reason for the same key: still nothing
  CHECK_FALSE(report_ignored_key_once(key, "Key " + key + " 2", "other."));
  std::vector<std::thread> threads;
  for (int t = 0; t < 8; ++t)
    threads.emplace_back(
        [&key] { report_ignored_key_once(key, "Key " + key, "why."); });
  for (auto &t : threads) t.join();
  CHECK(cap.err() == line);

  // another key has its own notice
  const std::string other = "TEST_NOTICE_ONCE_OTHER";
  REQUIRE_FALSE(ignored_key_reported(other));
  CHECK(report_ignored_key_once(other, "Key " + other, "why."));
  CHECK(count_of(cap.err(), "is ignored") == 2);
}
