/**
 * @file blueprint_validator.cpp
 * @brief Implements the BlueprintValidator class and associated logic 
 *        for blueprint-based matching, ensuring no memory leaks.
 *
 * @details
 * Also contains a demonstration 'main()' if compiled with a dedicated macro
 * to replicate the same output as the original code, using toml11 for reading
 * the "schema.toml" patterns/valid/invalid examples.
 */

#include <tadah/core/blueprint_validator.h>

#include <regex>
#include <unordered_map>
#include <algorithm>
#include <toml.hpp>

namespace tadah {
namespace core {

// ---------------------------------------------------------------------------
//                    Section: Internal Helpers
// ---------------------------------------------------------------------------

/// Internal mapping from token names to partial regex.
static const std::unordered_map<std::string, std::string> g_primitiveMap = {
  {"INT",    "[-+]?\\d+"},
  {"DOUBLE", "[-+]?\\d+(?:\\.\\d+)?(?:[eE][-+]?\\d+)?"},
  {"STRING", "(?=^.*[A-Za-z].*$)[^\\s]+"},
  {"UINT",   "(?:0|[1-9]\\d*)"},
  {"BOOL",   "(?:true|false)"}
};

/**
 * @brief Helper that trims leading/trailing whitespace from a string.
 */
static std::string trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return "";
  size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

/**
 * @brief Splits a blueprint string into tokens, including bracketed or braced groups.
 */
static std::vector<std::string> splitBlueprint(const std::string &blueprint) {
  std::vector<std::string> tokens;
  std::string token;
  bool inGroup = false;
  char groupEnd = '\0';

  for (char c : blueprint) {
    if (!inGroup && (c == '[' || c == '{')) {
      inGroup = true;
      groupEnd = (c == '[') ? ']' : '}';
      if (!token.empty()) {
        tokens.push_back(trim(token));
        token.clear();
      }
      token.push_back(c);
    } else if (inGroup) {
      token.push_back(c);
      if (c == groupEnd) {
        inGroup = false;
        tokens.push_back(trim(token));
        token.clear();
      }
    } else {
      if (std::isspace(static_cast<unsigned char>(c))) {
        if (!token.empty()) {
          tokens.push_back(trim(token));
          token.clear();
        }
      } else {
        token.push_back(c);
      }
    }
  }
  if (!token.empty()) {
    tokens.push_back(trim(token));
  }
  return tokens;
}

// ----------------------------------------------------------------------------
// reorderAlternatives Implementation
// ----------------------------------------------------------------------------
std::vector<std::string> reorderAlternatives(const std::vector<std::string> &alts) {
  // rank: 1 -> UINT, 2 -> INT, 3 -> DOUBLE, everything else -> 9999
  auto rank = [](const std::string &s) -> int {
    if (s == "UINT")   return 1;
    if (s == "INT")    return 2;
    if (s == "DOUBLE") return 3;
    return 9999; // e.g. BOOL, STRING, etc. remain after numeric tokens
  };

  std::vector<std::string> result = alts;
  // stable bubble-like approach
  for (size_t i = 0; i + 1 < result.size(); ++i) {
    for (size_t j = 0; j + 1 < result.size() - i; ++j) {
      if (rank(result[j]) > rank(result[j + 1])) {
        std::swap(result[j], result[j + 1]);
      }
    }
  }
  return result;
}

// Forward declarations
static std::string buildTokenRegex(const std::string &token);
static std::string buildRegexFromBlueprint(const std::string &blueprint);

/**
 * @brief Builds repeated group pattern for tokens in curly braces { ... }.
 *
 * E.g. {INT STRING} -> single-block: "INT\s+STRING"
 * Repetition: "(?:INT\s+STRING)(?:\s+INT\s+STRING)*"
 */
static std::string buildRepeatedBlock(const std::vector<std::string> &innerTokens) {
  if (innerTokens.empty()) {
    return "";
  }
  // build single block
  std::string block;
  bool first = true;
  for (auto &tok : innerTokens) {
    if (!first) block += "\\s+";
    block += buildTokenRegex(tok);
    first = false;
  }
  // now repeat it
  return "(?:" + block + ")(?:\\s+" + block + ")*";
}

/**
 * @brief Builds the partial pattern for a single token
 *        (repeated group, optional group, or a required token).
 */
static std::string buildTokenRegex(const std::string &token) {
  if (token.empty()) return "";

  // repeated group
  if (token.front() == '{' && token.back() == '}') {
    std::string inner = trim(token.substr(1, token.size() - 2));
    auto innerTokens = splitBlueprint(inner);
    return buildRepeatedBlock(innerTokens);
  }
  // optional group
  else if (token.front() == '[' && token.back() == ']') {
    // remove brackets
    std::string inner = trim(token.substr(1, token.size() - 2));
    // check for " or "
    if (inner.find(" or ") != std::string::npos) {
      std::vector<std::string> alts;
      std::string s = inner;
      const std::string delim = " or ";
      size_t pos;
      while ((pos = s.find(delim)) != std::string::npos) {
        alts.push_back(trim(s.substr(0, pos)));
        s = s.substr(pos + delim.size());
      }
      alts.push_back(trim(s));
      alts = reorderAlternatives(alts);

      std::string altRegex = "(?:";
      for (size_t i = 0; i < alts.size(); ++i) {
        auto it = g_primitiveMap.find(alts[i]);
        if (it != g_primitiveMap.end()) altRegex += it->second;
        else altRegex += alts[i]; // fallback
        if (i + 1 < alts.size()) altRegex += "|";
      }
      altRegex += ")";
      return altRegex;
    } else {
      // single token
      auto it = g_primitiveMap.find(inner);
      return (it != g_primitiveMap.end()) ? it->second : inner;
    }
  }
  // required token
  else {
    auto it = g_primitiveMap.find(token);
    return (it != g_primitiveMap.end()) ? it->second : token;
  }
}

/**
 * @brief Builds final anchored pattern capturing each blueprint token in order.
 */
static std::string buildRegexFromBlueprint(const std::string &blueprint) {
  auto tokens = splitBlueprint(blueprint);
  std::string pattern = "^";
  bool first = true;
  for (auto &tok : tokens) {
    bool isOptional =
      (!tok.empty() && tok.front() == '[' && tok.back() == ']');
    if (first) {
      first = false;
      if (isOptional) {
        pattern += "(" + buildTokenRegex(tok) + ")?";
      } else {
        pattern += "(" + buildTokenRegex(tok) + ")";
      }
    } else {
      if (isOptional) {
        pattern += "(?:\\s+(" + buildTokenRegex(tok) + "))?";
      } else {
        pattern += "\\s+(" + buildTokenRegex(tok) + ")";
      }
    }
  }
  pattern += "\\s*$";
  return pattern;
}

// ---------------------------------------------------------------------------
// BlueprintValidator Implementation
// ---------------------------------------------------------------------------

/**
 * @class BlueprintValidator::Impl
 * @brief Hides the actual std::regex as a pImpl detail.
 */
class BlueprintValidator::Impl {
public:
  explicit Impl(const std::string &pattern)
    : compiled(pattern, std::regex::ECMAScript)
  {}
  std::regex compiled;
};

BlueprintValidator::BlueprintValidator(const std::string &blueprint)
  : m_pattern(buildRegexFromBlueprint(blueprint))
  , m_impl(new Impl(m_pattern))  // store compiled pattern in pImpl
{
}

bool BlueprintValidator::validate(const std::string &text) const {
  // If no pImpl, something is wrong, but typically won't happen in normal usage
  if (!m_impl) return false;
  return std::regex_match(text, m_impl->compiled);
}

std::vector<std::string> BlueprintValidator::getCaptures(const std::string &text) const {
  std::vector<std::string> matches;
  if (!m_impl) return matches;
  std::smatch m;
  if (std::regex_match(text, m, m_impl->compiled)) {
    // group 0 is entire match, skip that
    for (size_t i = 1; i < m.size(); ++i) {
      matches.push_back(m[i].str());
    }
  }
  return matches;
}

bool BlueprintValidator::validateOnce(const std::string &blueprint, const std::string &text) {
  std::string pat = buildRegexFromBlueprint(blueprint);
  std::regex r(pat, std::regex::ECMAScript);
  return std::regex_match(text, r);
}

std::string BlueprintValidator::getPattern() const {
  return m_pattern;
}

} // namespace core
} // namespace tadah
