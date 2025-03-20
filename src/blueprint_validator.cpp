/**
 * @file blueprint_validator.cpp
 * @brief Implements the BlueprintValidator class and associated logic
 *        for blueprint-based matching, ensuring no memory leaks
 *        and supporting both full and partial matches.
 *
 * This version handles:
 *   - Nested bracket logic (STRING {[INT | DOUBLE]} {STRING STRING}).
 *   - Correct repeated-block parsing for tokens in curly braces.
 *   - An always-failing pattern for empty blueprints.
 *   - Reordered numeric primitives (UINT, INT, DOUBLE).
 *   - Partial-match fallback in getCaptures().
 *
 * The code is C++11 compatible. BLAS/LAPACK calls could be integrated
 * if needed for heavier computations. Here, std::regex is used.
 */

#include <tadah/core/blueprint_validator.h>

#include <regex>
#include <unordered_map>
#include <algorithm>
#include <toml.hpp>

/*#include <iostream> // For debug if needed*/

namespace tadah {
namespace core {

// ---------------------------------------------------------------------------
// Internal Helpers
// ---------------------------------------------------------------------------

/// Internal mapping from token names to partial regex.
static const std::unordered_map<std::string, std::string> g_primitiveMap = {
  {"INT",    "[-+]?\\d+"},
  {"DOUBLE", "[-+]?\\d+(?:\\.\\d+)?(?:[eE][-+]?\\d+)?"},
  {"STRING", "[^\\s]*[A-Za-z*][^\\s]*"},  /* non-empty string with a letter */
  {"UINT",   "(?:0|[1-9]\\d*)"},           /* unsigned integer pattern */
  {"BOOL",   "(?:true|false)"}
};

/**
 * @brief Removes leading/trailing whitespace from a string.
 *
 * @param s Input string.
 * @return Trimmed version of s.
 */
static std::string trim(const std::string &s) {
  size_t start = s.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) return "";
  size_t end = s.find_last_not_of(" \t\r\n");
  return s.substr(start, end - start + 1);
}

/**
 * @brief Splits a "blueprint" string into tokens, recognizing bracketed or braced groups.
 *
 * The content of square or curly brackets is preserved as one token.
 *
 * @param blueprint The input blueprint string.
 * @return A vector of tokens.
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

/**
 * @brief Reorders numeric tokens so that "UINT" is tried first,
 *        then "INT", then "DOUBLE". Everything else preserves its relative order.
 *
 * This function can help disambiguate numeric outlines more efficiently.
 *
 * @param alts The list of alternatives that might include UINT, INT, DOUBLE, etc.
 * @return The reordered list of alternatives.
 */
static std::vector<std::string> reorderAlternatives(const std::vector<std::string> &alts) {
  auto rank = [](const std::string &s) -> int {
    if (s == "UINT")   return 1;  // highest priority
    if (s == "INT")    return 2;
    if (s == "DOUBLE") return 3;  // lowest among numeric
    return 9999;                  // everything else
  };

  std::vector<std::string> result = alts;
  // Simple bubble sort for potentially small lists
  for (size_t i = 0; i + 1 < result.size(); ++i) {
    for (size_t j = 0; j + 1 < result.size() - i; ++j) {
      if (rank(result[j]) > rank(result[j + 1])) {
        std::swap(result[j], result[j + 1]);
      }
    }
  }
  return result;
}

/**
 * @brief Builds the partial pattern for a single token:
 *        - Required token
 *        - Optional token in square brackets ([...])
 *        - Repeated token in curly braces ({...}).
 *
 * The function is the key to handling nested bracket usage.
 *
 * @param token A single token extracted from splitBlueprint().
 * @return The corresponding regex fragment.
 */
static std::string buildTokenRegex(const std::string &token);

/**
 * @brief Builds a final anchored regex pattern capturing each token in order,
 *        optionally collecting submatches along the way.
 *
 * If all tokens are optional or empty, returns a pattern that fails everything
 * to avoid matching an empty blueprint as `^\s*$`.
 *
 * @param blueprint The user-specified blueprint.
 * @return The anchored pattern for full matching.
 */
static std::string buildRegexFromBlueprint(const std::string &blueprint) {
  // Split top-level tokens
  std::vector<std::string> tokens = splitBlueprint(blueprint);
  if (tokens.empty()) {
    // Return an always-failing pattern if no tokens
    return "^(?!).*$";
  }

  std::string pattern = "^";
  bool first = true;
  for (auto &tok : tokens) {
    bool isOptional = (!tok.empty() && tok.front() == '[' && tok.back() == ']');
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
  /*std::cout << "[DEBUG] Blueprint " << blueprint << " -> " << pattern << std::endl;*/
  return pattern;
}

/**
 * @brief Implementation detail of buildTokenRegex.
 */
static std::string buildTokenRegex(const std::string &token) {
  if (token.empty()) {
    return "";
  }

  // Repeated group: {...}
  if (token.front() == '{' && token.back() == '}') {
    // Extract inside braces
    std::string inner = trim(token.substr(1, token.size() - 2));

    // Recursively parse sub-tokens via splitBlueprint
    // Each subtoken is processed (supports nested optional groups).
    std::vector<std::string> subTokens = splitBlueprint(inner);
    std::string singleBlock;
    bool first = true;
    for (auto &subTok : subTokens) {
      if (!first) {
        singleBlock += "\\s+";
      }
      // Recursively build the subtoken pattern
      singleBlock += buildTokenRegex(subTok);
      first = false;
    }
    // Wrap into repeating pattern
    return "(?:" + singleBlock + ")(?:\\s+" + singleBlock + ")*";
  }
  // Optional group: [...]
  else if (token.front() == '[' && token.back() == ']') {
    // e.g. [INT | DOUBLE]
    std::string inner = trim(token.substr(1, token.size() - 2));

    // Replace " or " with '|'
    {
      size_t pos = 0;
      while ((pos = inner.find(" or ", pos)) != std::string::npos) {
        inner.replace(pos, 4, "|");
        pos++;
      }
    }
    // Now split on '|'
    std::vector<std::string> alts;
    {
      size_t start = 0;
      while (true) {
        size_t barPos = inner.find('|', start);
        if (barPos == std::string::npos) {
          std::string piece = trim(inner.substr(start));
          if (!piece.empty()) {
            alts.push_back(piece);
          }
          break;
        } else {
          std::string piece = trim(inner.substr(start, barPos - start));
          if (!piece.empty()) {
            alts.push_back(piece);
          }
          start = barPos + 1;
        }
      }
    }
    // Reorder numeric tokens if multiple
    if (alts.size() > 1) {
      alts = reorderAlternatives(alts);
      std::string altRegex = "(?:";
      for (size_t i = 0; i < alts.size(); ++i) {
        auto it = g_primitiveMap.find(alts[i]);
        if (it != g_primitiveMap.end()) {
          altRegex += it->second;
        } else {
          altRegex += alts[i];
        }
        if (i + 1 < alts.size()) {
          altRegex += "|";
        }
      }
      altRegex += ")";
      return altRegex;
    } else if (!alts.empty()) {
      // Single alternative if exactly one found
      auto x = g_primitiveMap.find(alts[0]);
      return (x != g_primitiveMap.end()) ? x->second : alts[0];
    }
    // If empty, return empty
    return "";
  }
  // Otherwise, required token (primitive or literal)
  else {
    auto it = g_primitiveMap.find(token);
    return (it != g_primitiveMap.end()) ? it->second : token;
  }
}

// ---------------------------------------------------------------------------
// BlueprintValidator Implementation (supports multiple blueprints)
// ---------------------------------------------------------------------------

/**
 * @class BlueprintValidator::Impl
 * @brief Hides the actual std::regex as a pImpl detail, storing multiple patterns.
 */
class BlueprintValidator::Impl {
public:
  Impl() = default;

  /**
   * @brief Constructs and stores all patterns as separate regex objects in a list.
   */
  explicit Impl(const std::vector<std::string> &patterns) {
    compiledPatterns.reserve(patterns.size());
    for (auto &p : patterns) {
      compiledPatterns.emplace_back(std::regex(p, std::regex::ECMAScript));
    }
  }

  std::vector<std::regex> compiledPatterns;
};

BlueprintValidator::BlueprintValidator(const std::string &blueprint)
  : m_patterns(""), m_impl(nullptr) {
  // Build single blueprint pattern
  std::string pat = buildRegexFromBlueprint(blueprint);
  m_patterns = pat;

  std::vector<std::string> compiled;
  compiled.push_back(pat);
  m_impl = new Impl(compiled);
}

BlueprintValidator::BlueprintValidator(const std::vector<std::string> &blueprints)
  : m_patterns(""), m_impl(nullptr) {
  bool first = true;
  std::vector<std::string> compiled;
  compiled.reserve(blueprints.size());

  // Build each blueprint, compile them, and store altogether.
  for (auto &bp : blueprints) {
    std::string pat = buildRegexFromBlueprint(bp);
    if (!first) {
      m_patterns += "\n";
    }
    m_patterns += pat;
    compiled.push_back(pat);
    first = false;
  }
  m_impl = new Impl(compiled);
}

bool BlueprintValidator::validate(const std::string &text) const {
  // If no pImpl, no match is possible
  if (!m_impl) {
    return false;
  }
  // Attempts to match text with any compiled pattern
  for (auto &rgx : m_impl->compiledPatterns) {
    if (std::regex_match(text, rgx)) {
      return true;
    }
  }
  return false;
}

/**
 * @brief Retrieves captures by checking a set of regex patterns.
 *
 * Returns captures for the first regex pattern that fully matches.
 * If no full match is found, returns captures for the pattern
 * with the largest partial match. The first submatch is always the entire match,
 * so real captures start at index 1.
 *
 * @param text The input text to be matched against the stored patterns.
 * @return A vector of captured substrings from either the first full match
 *         or, if none, from the best partial match.
 */
std::vector<std::string> BlueprintValidator::getCaptures(const std::string &text) const {
  std::vector<std::string> matches;
  if (!m_impl) {
    return matches;
  }

  // Tracks best partial match if full matches are not found
  int bestPartialSize = -1;
  std::smatch bestPartialMatch;

  // Tries each pattern for a full match first
  for (auto &rgx : m_impl->compiledPatterns) {
    std::smatch m;
    if (std::regex_match(text, m, rgx)) {
      // Captures for the first full match
      for (size_t i = 1; i < m.size(); ++i) {
        matches.push_back(m[i].str());
      }
      break; // Skips others if matched
    } else {
      // Partial match searching
      if (std::regex_search(text, m, rgx)) {
        int partialSize = static_cast<int>(m[0].length());
        if (partialSize > bestPartialSize) {
          bestPartialSize = partialSize;
          bestPartialMatch = m;
        }
      }
    }
  }

  // If no full match was found, returns captures from the best partial match
  if (matches.empty() && bestPartialSize > 0) {
    for (size_t i = 1; i < bestPartialMatch.size(); ++i) {
      matches.push_back(bestPartialMatch[i].str());
    }
  }

  return matches;
}

/* Alternative simpler getCaptures version if partial match is not needed:

std::vector<std::string> BlueprintValidator::getCaptures(const std::string &text) const {
  std::vector<std::string> matches;
  if (!m_impl) {
    return matches;
  }
  // For multiple patterns, returns captures for the first that matches
  for (auto &rgx : m_impl->compiledPatterns) {
    std::smatch m;
    if (std::regex_match(text, m, rgx)) {
      for (size_t i = 1; i < m.size(); ++i) {
        matches.push_back(m[i].str());
      }
      break; // Only return first match's captures
    }
  }
  return matches;
}
*/

bool BlueprintValidator::validateOnce(const std::string &blueprint, const std::string &text) {
  std::string pat = buildRegexFromBlueprint(blueprint);
  std::regex r(pat, std::regex::ECMAScript);
  return std::regex_match(text, r);
}

bool BlueprintValidator::validateOnce(const std::vector<std::string> &blueprints, const std::string &text) {
  BlueprintValidator v(blueprints);
  return v.validate(text);
}

std::string BlueprintValidator::getPattern() const {
  return m_patterns;
}

} // namespace core
} // namespace tadah

