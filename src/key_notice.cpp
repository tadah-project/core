#include "tadah/core/key_notice.h"

#include <iostream>
#include <mutex>
#include <unordered_set>

namespace tadah {
namespace core {

namespace {
/* The keys report_ignored_key_once() has reported in this process. */
std::mutex &reported_mutex() {
  static std::mutex m;
  return m;
}
std::unordered_set<std::string> &reported_keys() {
  static std::unordered_set<std::string> s;
  return s;
}
} // namespace

void report_ignored_key(const std::string &subject, const std::string &reason) {
  std::cerr << ("WARNING: " + subject + " is ignored: " + reason + "\n");
}

bool report_ignored_key_once(const std::string &key, const std::string &subject,
                             const std::string &reason) {
  {
    std::lock_guard<std::mutex> lock(reported_mutex());
    if (!reported_keys().insert(key).second) return false;
  }
  report_ignored_key(subject, reason);
  return true;
}

bool ignored_key_reported(const std::string &key) {
  std::lock_guard<std::mutex> lock(reported_mutex());
  return reported_keys().count(key) != 0;
}

} // namespace core
} // namespace tadah
