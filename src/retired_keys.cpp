#include <tadah/core/retired_keys.h>

#include <algorithm>
#include <stdexcept>

namespace tadah {
namespace core {

namespace {

/* "line 2 of old.tadah", or "line 2" for a line without a file. */
std::string where(const RetiredKeyLine &l) {
  std::string out = "line " + std::to_string(l.line);
  if (!l.source.empty()) out += " of " + l.source;
  return out;
}

/* "a", "a and b", "a, b and c". */
std::string join_and(const std::vector<std::string> &items) {
  std::string out;
  for (std::size_t i = 0; i < items.size(); ++i) {
    if (i > 0) out += i + 1 == items.size() ? " and " : ", ";
    out += items[i];
  }
  return out;
}

/* The text with its whitespace runs (line breaks of the blueprint's
 * multi-line strings included) made single blanks, without blanks around
 * it. */
std::string one_line(const std::string &text) {
  std::string out;
  bool blank = false;
  for (char ch : text) {
    if (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r') {
      blank = !out.empty();
      continue;
    }
    if (blank) out += ' ';
    blank = false;
    out += ch;
  }
  return out;
}

const BlueprintKey &retired_entry(const std::string &key) {
  const auto &keys = BlueprintParser::getKeysMapRef();
  const auto it = keys.find(key);
  if (it == keys.end() || !it->second.isRetired())
    throw std::logic_error("retired_keys: " + key + " is not a retired key");
  return it->second;
}

}  // namespace

std::string retired_key_text(const std::string &key, const BlueprintKey &info) {
  return key + " is a retired key: " + one_line(info.retired.reason) +
         " Replaced by: " + one_line(info.retired.replacement) + ". " +
         one_line(info.retired.conversion);
}

std::string retired_keys_notice(const std::vector<RetiredKeyLine> &lines) {
  std::vector<std::string> named;
  std::vector<std::string> keys;
  for (const auto &l : lines) {
    named.push_back(l.key + " (" + where(l) + ")");
    if (std::find(keys.begin(), keys.end(), l.key) == keys.end())
      keys.push_back(l.key);
  }
  const bool one = lines.size() == 1;
  std::vector<std::string> explain;
  for (const auto &k : keys) explain.push_back("tadah explain " + k);
  return std::string(one ? "Retired key " : "Retired keys ") +
         join_and(named) +
         (one ? " is ignored: it weighted the training data of a fit before "
                "Tadah! 1.6.0 and changes nothing a prediction gives; a fit "
                "refuses it ("
              : " are ignored: they weighted the training data of a fit "
                "before Tadah! 1.6.0 and change nothing a prediction gives; "
                "a fit refuses them (") +
         join_and(explain) +
         (keys.size() == 1 ? " gives the conversion)."
                           : " give the conversions).");
}

std::string retired_keys_refusal(const std::vector<RetiredKeyLine> &lines,
                                 const std::string &who) {
  std::vector<std::string> named;
  std::vector<std::string> keys;
  for (const auto &l : lines) {
    std::string text = l.key;
    for (const auto &v : l.values) text += " " + v;
    named.push_back(text + " (" + where(l) + ")");
    if (std::find(keys.begin(), keys.end(), l.key) == keys.end())
      keys.push_back(l.key);
  }
  std::string msg = who + ": the configuration has " +
                    (lines.size() == 1 ? "a key" : "keys") +
                    " retired in Tadah! 1.6.0, and a fit refuses " +
                    (lines.size() == 1 ? "it" : "them") + ": " +
                    join_and(named) + ".";
  for (const auto &k : keys) {
    const BlueprintKey &info = retired_entry(k);
    msg += " " + k + ": " + one_line(info.retired.conversion);
  }
  msg += " With the converted keys in place of " +
         std::string(lines.size() == 1 ? "this line" : "these lines") +
         " (in the file that has " +
         std::string(lines.size() == 1 ? "it" : "them") +
         ", a potential's included), the configuration trains; no "
         "coefficients were fitted.";
  return msg;
}

void require_no_retired_keys(const Context &context, const std::string &who) {
  const std::vector<RetiredKeyLine> lines = context.retired_keys();
  if (!lines.empty())
    throw std::runtime_error(retired_keys_refusal(lines, who));
}

}  // namespace core
}  // namespace tadah
