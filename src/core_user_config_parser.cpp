#include "tadah/core/blueprint_parser.h"
#include "tadah/core/blueprint_validator.h"
#include "tadah/core/core_user_config_parser.h"
#include "tadah/core/key_notice.h"
#include "tadah/core/logger.h"
#include "tadah/core/utils.h"
#include <cctype>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace {
/* "A, B, C": the allowed values of a key, for its error messages. */
std::string joinAllowed(const std::vector<std::string> &values) {
  std::string out;
  for (std::size_t i = 0; i < values.size(); ++i)
    out += (i ? ", " : "") + values[i];
  return out;
}
} // namespace

namespace tadah {
namespace core {

namespace {
  /**
   * @brief Holds the recursion depth of parse() to identify top-level calls.
   *
   * parse() might be called recursively when encountering nested CONFIG lines.
   * If parseDepth is zero, it means a top-level parse() call starts.
   */
  static thread_local int parseDepth = 0;

  /**
   * @brief Tracks the chain of CONFIG files to detect circular references.
   *
   * On each nested parse for CONFIG, the file name is pushed. After that parse
   * finishes, it is popped. If parseDepth resets to zero, the chain is cleared.
   */
  static thread_local std::vector<std::string> configChain;

  /**
   * @brief Writes the notice for a deprecated key to standard error, once
   *        per key and process (report_ignored_key_once(), the rule of
   *        every ignored key: however many lines, CONFIG files or parses
   *        read it). Not a Logger message, so neither the Logger level nor
   *        a VERBOSE line hides it, in LAMMPS either.
   */
  void reportDeprecatedKey(const std::string &key, const std::string &reason) {
    report_ignored_key_once(key, "Deprecated key " + key, reason);
  }

  /**
   * @brief One parse() call: tracks the recursion depth and resets the
   *        per-parse state (the CONFIG chain) at the start and at the end
   *        of a top-level parse.
   *
   * RAII, so that a parse that throws (a malformed line, a missing CONFIG
   * file) cannot leave the depth raised: the next parse on this thread is
   * then a top-level parse again, with a fresh CONFIG chain.
   */
  class ParseScope {
  public:
    ParseScope() {
      if (parseDepth == 0) reset();
      ++parseDepth;
    }
    ~ParseScope() {
      --parseDepth;
      if (parseDepth == 0) reset();
    }
    ParseScope(const ParseScope &) = delete;
    ParseScope &operator=(const ParseScope &) = delete;

  private:
    static void reset() {
      configChain.clear();
    }
  };

  /**
   * @brief "line N" or "line N of <source>": where an error is.
   */
  std::string lineOf(std::size_t line, const std::string &source) {
    std::string out = "line " + std::to_string(line);
    if (!source.empty()) out += " of " + source;
    return out;
  }

  /**
   * @brief The keys whose lines open a verbatim multi-line block (closed by
   *        END<key>), whose body is not made of key lines.
   */
  const std::unordered_set<std::string> &verbatimBlocks() {
    static const std::unordered_set<std::string> blocks = {
        "OPTIMIZER", "INIT", "EXPLORE", "REFINE", "FUSE"};
    return blocks;
  }

  /**
   * @brief The keys of the key lines of @p lines (KeyLineRule's file_keys):
   *        the first word, upper case, of each line that is neither blank
   *        nor a comment, the lines inside a verbatim block excepted (the
   *        block's own name is one of them).
   */
  std::set<std::string> fileKeys(const std::vector<std::string> &lines) {
    std::set<std::string> keys;
    std::string blockEnd;  // the END<block> awaited; empty outside a block
    for (const std::string &raw : lines) {
      std::string line = trim(raw);
      if (line.empty() || line[0] == '#') continue;
      line = trim(line.substr(0, line.find('#')));
      if (line.empty()) continue;
      if (!blockEnd.empty()) {
        if (to_upper(line) == blockEnd) blockEnd.clear();
        continue;
      }
      std::istringstream iss(line);
      std::string first;
      iss >> first;
      const std::string uKey = to_upper(first);
      keys.insert(uKey);
      if (verbatimBlocks().count(uKey)) blockEnd = "END" + uKey;
    }
    return keys;
  }

  /**
   * @brief The key line rule of a parse and the list its skipped lines go
   *        to (both null: no rule).
   */
  struct LineSkip {
    const KeyLineRule *rule = nullptr;
    std::vector<SkippedKeyLine> *skipped = nullptr;
  };

  Context parseFile(const std::string &filename,
                    const std::map<std::string, std::string> &refused,
                    const LineSkip &skip);

  /**
   * @brief Parse lines into an existing Context object.
   *
   * lines[i] is line first_line + i of source (empty: no name), which the
   * errors that name a line report.
   *
   * Circular dependency detection is handled by tracking CONFIG filenames
   * in configChain. When a CONFIG line is encountered, the file is pushed
   * into configChain, parsed, then popped after the nested parse.
   *
   * With a rule (@p skip), a key line the rule selects by the keys of
   * @p lines (fileKeys) is checked, not stored, and appended to
   * skip.skipped; a CONFIG include is read under the same rule.
   */
  void parseLines(const std::vector<std::string> &lines, Context &ctx,
                  std::size_t first_line, const std::string &source,
                  const std::map<std::string, std::string> &refused,
                  const LineSkip &skip = LineSkip()) {
    const auto keyMap = BlueprintParser::getKeysMapRef();
    const std::set<std::string> ownKeys =
        skip.rule ? fileKeys(lines) : std::set<std::string>();
    for (size_t i = 0; i < lines.size(); ++i) {
      std::string rawLine = trim(lines[i]);
      if (rawLine.empty() || rawLine[0] == '#') {
        continue;
      }
      // Remove in-line comments, and the blanks before them: the value of a
      // key is the text up to the last non-blank character before the '#'
      // (CONFIG uses it as a path, also in its circular-include check).
      rawLine = trim(rawLine.substr(0, rawLine.find('#')));
      std::istringstream iss(rawLine);
      std::vector<std::string> tokens;
      {
        std::string token;
        while (iss >> token) {
          tokens.push_back(token);
        }
      }
      if (tokens.empty()) {
        continue;
      }

      // first token is the key
      const std::string &key = tokens[0];
      std::string uKey = to_upper(key);

      // A key the caller refuses for this input (refuse_keys()).
      if (const auto r = refused.find(uKey); r != refused.end()) {
        throw std::runtime_error("Key " + uKey + " on " +
                                 lineOf(first_line + i, source) +
                                 " is not accepted here: " + r->second);
      }

      // Verbatim multi-line blocks (not flat keys). Slurp every token between
      // `<BLOCK>` and the matching `END<BLOCK>` into the single (unbounded)
      // Context STRINGS key <BLOCK>; the consuming engine (e.g. `tadah refit`)
      // parses that token stream itself. Mirrors the CONFIG special-case below.
      //   OPTIMIZER  - legacy refit optimiser block (now nested; flagged by
      //                the refit parser if it appears at top level)
      //   INIT/EXPLORE/REFINE/FUSE - the refit stage blocks. A stage's nested
      //                OPTIMIZER...ENDOPTIMIZER is captured as body tokens
      //                because each opener closes ONLY on its own END<opener>
      //                (distinct end tokens => no depth counter needed).
      if (verbatimBlocks().count(uKey)) {
        const std::string endTok = "END" + uKey;
        std::string body;
        bool closed = false;
        size_t k = i + 1;
        for (; k < lines.size(); ++k) {
          std::string bl = trim(lines[k]);
          if (bl.empty() || bl[0] == '#') {
            continue;
          }
          bl = trim(bl.substr(0, bl.find('#')));
          if (bl.empty()) {
            continue;
          }
          if (to_upper(bl) == endTok) {
            closed = true;
            break;
          }
          body += bl + " ";
        }
        if (!closed) {
          throw std::runtime_error(uKey + " block is not closed with " +
                                   endTok);
        }
        i = k;  // resume after END<block>
        if (!body.empty()) {
          ctx.add(uKey, body);  // STRINGS key: add() splits on whitespace
        }
        continue;
      }

      if (keyMap.find(uKey) == keyMap.end()) {
        std::string suggested = BlueprintParser::findClosestKey(key, keyMap);
        Logger::instance().log(
            LogLevel::ERROR, "CoreUserConfigParser::parse -> Invalid key: ", key);
        if (!suggested.empty()) {
          Logger::instance().log(LogLevel::ERROR, "  Did you mean: ", suggested);
        }
        throw std::runtime_error("Invalid key: " + key);
      }
      // validate entry against blueprint schema
      std::string val = removeFirstWord(rawLine, key);
      const BlueprintKey &keyInfo = keyMap.at(uKey);
      bool valid_schema = CoreUserConfigParser::validateWithBlueprint(key, val, keyInfo);
      if (!valid_schema) {
        // A value outside the key's allowed_values: name the line.
        if (!keyInfo.allowed_values.empty() &&
            CoreUserConfigParser::validateSchemaOnly(val, keyInfo)) {
          throw std::runtime_error(
              "Invalid value for key " + uKey + " on " +
              lineOf(first_line + i, source) + ": '" + trim(val) +
              "' (allowed: " + joinAllowed(keyInfo.allowed_values) + ")");
        }
        // Values joined by a comma or a '+', which the line is not split
        // at: name the line, the separator and the line to write.
        std::string respaced;
        const std::string seps =
            CoreUserConfigParser::misusedSeparators(val, keyInfo, respaced);
        if (!seps.empty()) {
          throw std::runtime_error(
              "Invalid value for key " + uKey + " on " +
              lineOf(first_line + i, source) + ": '" + trim(val) +
              "' separates values with " + seps +
              ", which a configuration line is not split at (values are "
              "separated by blanks: " + uKey + " " + respaced + ")");
        }
        throw std::runtime_error("Invalid schema for key: " + key);
      }
      if (keyInfo.isDeprecated()) {
        // Accepted so that older files still load, but never stored: the
        // key has no effect and must not be written back to a potential.
        reportDeprecatedKey(uKey, keyInfo.deprecated);
        continue;
      }
      // A line the caller's rule does not read (checked above as any line).
      if (skip.rule && uKey != "CONFIG" && (*skip.rule)(ownKeys, uKey)) {
        skip.skipped->push_back(SkippedKeyLine{
            source, first_line + i, uKey,
            std::vector<std::string>(tokens.begin() + 1, tokens.end())});
        continue;
      }
      if (uKey == "CONFIG") {
        // CONFIG is a special case; check for circular dependency
        if (std::find(configChain.begin(), configChain.end(), val) != configChain.end()) {
          std::string chainStr;
          for (const auto &c : configChain) {
            chainStr += c + " -> ";
          }
          chainStr += val;
          throw std::runtime_error("Circular dependency detected between CONFIG files: " + chainStr);
        }
        // Push config name before nested parse
        configChain.push_back(val);

        // Perform nested parse, under the same refusal and rule
        Context configCtx;
        try {
          configCtx = parseFile(val, refused, skip);
        } catch (...) {
          // Clean up the chain if parsing fails
          configChain.pop_back();
          throw;
        }

        // Pop config name after nested parse
        configChain.pop_back();
        
        // Merge the nested context
        ctx = tadah::core::Context::merge(ctx, configCtx);
        continue; // Skip adding "CONFIG" key or its value to the context
      }
      // Other keys store their values
      for (size_t j = 1; j < tokens.size(); ++j) {
        ctx.add(key, tokens[j]);
      }
    }
  }

  /**
   * @brief One parse of the file @p filename: its lines, under the
   *        refusal @p refused and the rule @p skip (a top-level parse, or
   *        the file of a CONFIG line).
   */
  Context parseFile(const std::string &filename,
                    const std::map<std::string, std::string> &refused,
                    const LineSkip &skip) {
    ParseScope scope;

    std::ifstream infile(filename.c_str());
    if (!infile.is_open()) {
      Logger::instance().log(
          LogLevel::ERROR,
          "CoreUserConfigParser::parse -> Failed to open file: ", filename);
      throw std::runtime_error("Failed to open config file: " + filename);
    }

    std::vector<std::string> lines;
    {
      std::string line;
      while (std::getline(infile, line)) {
        lines.push_back(line);
      }
    }
    infile.close();

    Context ctx;
    parseLines(lines, ctx, 1, filename, refused, skip);
    return ctx;
  }
} // end anonymous namespace

Context CoreUserConfigParser::parse(const std::string &filename) const {
  return parseFile(filename, refused_, LineSkip());
}

Context CoreUserConfigParser::parse(const std::string &filename,
                                    const KeyLineRule &skip,
                                    std::vector<SkippedKeyLine> &skipped) const {
  return parseFile(filename, refused_,
                   skip ? LineSkip{&skip, &skipped} : LineSkip());
}

Context CoreUserConfigParser::parse(std::istream &in) const {
  ParseScope scope;

  std::vector<std::string> lines;
  {
    std::string line;
    while (std::getline(in, line)) {
      lines.push_back(line);
    }
  }

  Context ctx;
  parseLines(lines, ctx, 1, "", refused_);
  return ctx;
}

Context
CoreUserConfigParser::parse(const std::vector<std::string> &lines) const {
  ParseScope scope;

  Context ctx;
  parseLines(lines, ctx, 1, "", refused_);
  return ctx;
}

Context CoreUserConfigParser::parse(const std::vector<std::string> &lines,
                                    std::size_t first_line,
                                    const std::string &source) const {
  ParseScope scope;

  Context ctx;
  parseLines(lines, ctx, first_line, source, refused_);
  return ctx;
}

Context CoreUserConfigParser::parse(const std::vector<std::string> &lines,
                                    std::size_t first_line,
                                    const std::string &source,
                                    const KeyLineRule &skip,
                                    std::vector<SkippedKeyLine> &skipped) const {
  ParseScope scope;

  Context ctx;
  parseLines(lines, ctx, first_line, source, refused_,
             skip ? LineSkip{&skip, &skipped} : LineSkip());
  return ctx;
}

void CoreUserConfigParser::refuse_keys(
    std::map<std::string, std::string> reasons) {
  refused_.clear();
  for (auto &kv : reasons)
    refused_[to_upper(kv.first)] = std::move(kv.second);
}

bool CoreUserConfigParser::validateWithBlueprint(
    const std::string &key,
    const std::string &val,
    const BlueprintKey &keyInfo)
{
  return validateWithBlueprintImpl(key, val, keyInfo);
}

bool CoreUserConfigParser::validateWithBlueprint(
    const std::string &key,
    const std::vector<std::string> &vals,
    const BlueprintKey &keyInfo)
{
  std::string concatenated = core::concat(vals);
  return validateWithBlueprintImpl(key, concatenated, keyInfo);
}

bool CoreUserConfigParser::validateWithBlueprintImpl(
    const std::string &key,
    const std::string &text,
    const BlueprintKey &keyInfo)
{
  // 1. Validate against the key's schema, then its allowed values
  //    (blueprint field `allowed_values`, matched without regard to case).
  if (validateSchemaOnly(text, keyInfo)) {
    if (keyInfo.isAllowedValue(trim(text))) {
      return true;
    }
    Logger::instance().log(
        LogLevel::ERROR,
        "CoreUserConfigParser::parse -> Invalid value for key: ", key, ": ",
        text);
    Logger::instance().log(LogLevel::ERROR, "  Allowed values: ",
                           joinAllowed(keyInfo.allowed_values));
    return false;
  }

  // 3. If validation fails, log details once:
  Logger::instance().log(
      LogLevel::ERROR,
      "CoreUserConfigParser::parse -> Invalid format for key: value -> ",
      key, ": ", text);

  // Log allowed schemas
  Logger::instance().log(LogLevel::ERROR, "  Allowed schema:");
  for (const auto &schema : keyInfo.value_type) {
    Logger::instance().log(LogLevel::ERROR, "    ", key, "   ", schema);
  }

  // Log user-friendly formats
  Logger::instance().log(LogLevel::ERROR, "  Format:");
  for (const auto &fmt : keyInfo.value_format) {
    Logger::instance().log(LogLevel::ERROR, "    ", key, "   ", fmt);
  }

  // Provide examples if available. An example is a value of the key (the
  // blueprint's rule): each of its lines is shown as a line of the key.
  if (!keyInfo.examples.empty()) {
    Logger::instance().log(LogLevel::ERROR, "  Examples:");
    for (const auto &ex : keyInfo.examples) {
      std::istringstream lines(ex);
      std::string line;
      while (std::getline(lines, line)) {
        line = trim(line);
        if (!line.empty()) {
          Logger::instance().log(LogLevel::ERROR, "    ", key, "   ", line);
        }
      }
    }
  }

  // 4. Indicate failure
  return false;
}

namespace {
bool isBlankChar(char c) {
  return std::isspace(static_cast<unsigned char>(c)) != 0;
}
bool isDigitChar(char c) { return c >= '0' && c <= '9'; }

/* Whether text[i] is a '+' joined to the character before it, which no
 * value of a configuration line holds: not a sign after a blank or a comma,
 * nor an exponent's sign (an e or an E after a digit or a point, with a
 * digit after the '+': 1e+5, 2.5E+3). */
bool joiningPlus(const std::string &text, std::size_t i) {
  if (text[i] != '+' || i == 0) return false;
  const char before = text[i - 1];
  if (isBlankChar(before) || before == ',') return false;
  const bool exponent =
      (before == 'e' || before == 'E') && i >= 2 &&
      (isDigitChar(text[i - 2]) || text[i - 2] == '.') && i + 1 < text.size() &&
      isDigitChar(text[i + 1]);
  return !exponent;
}

/* Whether text holds a comma or a joining '+'. */
bool holdsSeparator(const std::string &text) {
  for (std::size_t i = 0; i < text.size(); ++i)
    if (text[i] == ',' || joiningPlus(text, i)) return true;
  return false;
}
}  // namespace

bool CoreUserConfigParser::validateSchemaOnly(const std::string &text,
                                              const BlueprintKey &keyInfo)
{
  // A name holds no ',' and no '+': there they separate two names.
  if (keyInfo.names && holdsSeparator(text)) return false;
  BlueprintValidator validator(keyInfo.value_type);
  return validator.validate(text);
}

std::string CoreUserConfigParser::misusedSeparators(const std::string &text,
                                                    const BlueprintKey &keyInfo,
                                                    std::string &respaced)
{
  struct Attempt {
    bool commas, pluses;
    const char *name;
  };
  const Attempt attempts[] = {{true, false, "','"},
                              {false, true, "'+'"},
                              {true, true, "',' and '+'"}};
  for (const Attempt &a : attempts) {
    std::string candidate = text;
    bool replaced_comma = false, replaced_plus = false;
    for (std::size_t i = 0; i < text.size(); ++i) {
      if (a.commas && text[i] == ',') {
        candidate[i] = ' ';
        replaced_comma = true;
      } else if (a.pluses && joiningPlus(text, i)) {
        candidate[i] = ' ';
        replaced_plus = true;
      }
    }
    if (replaced_comma != a.commas || replaced_plus != a.pluses) continue;
    if (!validateSchemaOnly(candidate, keyInfo) ||
        !keyInfo.isAllowedValue(trim(candidate)))
      continue;
    std::istringstream words(candidate);
    std::string word, joined;
    while (words >> word) joined += (joined.empty() ? "" : " ") + word;
    respaced = joined;
    return a.name;
  }
  return "";
}

} // namespace core
} // namespace tadah
