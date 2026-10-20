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

/* Whether @p context is M_KRR implementation 2 (MODEL M_KRR <kernel> 2,
 * the third value read as MLIP's is_krr_implementation_2 reads it), whose
 * solve in Tadah! 1.0.0 to 1.5.0, (K + LAMBDA I) w = t on the energies per
 * atom in eV, read none of the retired keys: no weight and no priority of
 * the data. false without a context. */
bool krr_implementation_2(const Context *context) {
  if (!context || !context->has_stored("MODEL") ||
      context->size("MODEL") < 3)
    return false;
  try {
    return context->get<std::string>("MODEL", 0) == "M_KRR" &&
           context->get<int>("MODEL", 2) == 2;
  } catch (const std::exception &) {
    return false;  // not an integer: the fit refuses the MODEL line
  }
}

/* "; FORCE false here: converted by deleting the line" for a line whose
 * conversion is its deletion: every line under M_KRR implementation 2
 * (@p krr2), FWEIGHT and SWEIGHT of a quantity that @p context leaves out
 * (a fit refuses an FSIGMA or SSIGMA other than its default then), and
 * WDBFILE_AUTO 0; "" for another line or without a context (the switches
 * are then unknown). */
std::string deleted_by(const RetiredKeyLine &l, const Context *context,
                       bool krr2) {
  const std::string tail = "converted by deleting the line";
  if (krr2) return "; M_KRR implementation 2 here: " + tail;
  if (l.key == "WDBFILE_AUTO" && l.values.size() == 1) {
    try {
      if (std::stod(l.values.front()) == 0.0) return "; 0: " + tail;
    } catch (const std::exception &) {
      // not a number: the parse refused it before
    }
    return "";
  }
  if (!context) return "";
  if (l.key == "FWEIGHT" && !context->get<bool>("FORCE"))
    return "; FORCE false here: " + tail;
  if (l.key == "SWEIGHT" && !context->get<bool>("STRESS"))
    return "; STRESS false here: " + tail;
  return "";
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
                                 const std::string &who,
                                 const Context *context) {
  const bool krr2 = krr_implementation_2(context);
  const bool one = lines.size() == 1;
  std::vector<std::string> named;
  std::vector<std::string> keys;
  for (const auto &l : lines) {
    std::string text = l.key;
    for (const auto &v : l.values) text += " " + v;
    named.push_back(text + " (" + where(l) + deleted_by(l, context, krr2) +
                    ")");
    if (std::find(keys.begin(), keys.end(), l.key) == keys.end())
      keys.push_back(l.key);
  }
  const std::string lines_here = one ? "this line" : "these lines";
  const std::string file_of_lines =
      std::string(" (in the file that has ") + (one ? "it" : "them") +
      ", a potential's included";
  std::string msg = who + ": the configuration has " +
                    (one ? "a key" : "keys") +
                    " retired in Tadah! 1.6.0, and a fit refuses " +
                    (one ? "it" : "them") + ": " + join_and(named) + ".";
  if (krr2) {
    // The keys' own conversions are those of the other fits: under
    // implementation 2 they would change the fit.
    msg += std::string(" M_KRR implementation 2 of Tadah! 1.0.0 to 1.5.0 ") +
           (one ? "did not read it" : "read none of them") +
           ": it solved (K + LAMBDA I) w = t on the energies per atom in eV "
           "(ESIGMA 1000). With " + lines_here + " deleted" + file_of_lines +
           ") and ESIGMA 1000, or LAMBDA times 10^6 with ESIGMA 1, the "
           "configuration fits with the solve of Tadah! 1.5.0; no "
           "coefficients were fitted.";
    return msg;
  }
  for (const auto &k : keys) {
    const BlueprintKey &info = retired_entry(k);
    msg += " " + k + ": " + one_line(info.retired.conversion);
  }
  // A retrain does not read a potential's WDBFILE line, so the WDBFILE
  // values of a potential's WDBFILE_AUTO go elsewhere (its conversion).
  const bool wdbfile_auto =
      std::find(keys.begin(), keys.end(), "WDBFILE_AUTO") != keys.end();
  msg += " With the converted keys in place of " + lines_here +
         file_of_lines +
         (wdbfile_auto ? "; the WDBFILE values of a potential on the command "
                         "line or in a file that includes it"
                       : "") +
         "), the configuration trains; no coefficients were fitted.";
  return msg;
}

void require_no_retired_keys(const Context &context, const std::string &who) {
  const std::vector<RetiredKeyLine> lines = context.retired_keys();
  if (!lines.empty())
    throw std::runtime_error(retired_keys_refusal(lines, who, &context));
}

}  // namespace core
}  // namespace tadah
