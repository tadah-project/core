#include "tadah/core/blueprint_parser.h"
#include "tadah/core/blueprint_validator.h"
#include "tadah/core/core_user_config_parser.h"
#include "tadah/core/key_notice.h"
#include "tadah/core/logger.h"
#include "tadah/core/utils.h"
#include <fstream>
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
   * @brief Parse lines into an existing Context object.
   *
   * lines[i] is line first_line + i of source (empty: no name), which the
   * errors that name a line report.
   *
   * Circular dependency detection is handled by tracking CONFIG filenames
   * in configChain. When a CONFIG line is encountered, the file is pushed
   * into configChain, parsed, then popped after the nested parse.
   */
  void parseLines(const std::vector<std::string> &lines, Context &ctx,
                  std::size_t first_line, const std::string &source) {
    const auto keyMap = BlueprintParser::getKeysMapRef();
    for (size_t i = 0; i < lines.size(); ++i) {
      std::string rawLine = trim(lines[i]);
      if (rawLine.empty() || rawLine[0] == '#') {
        continue;
      }
      // Remove in-line comments
      rawLine = rawLine.substr(0, rawLine.find('#'));
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
      static const std::unordered_set<std::string> kVerbatimBlocks = {
          "OPTIMIZER", "INIT", "EXPLORE", "REFINE", "FUSE"};
      if (kVerbatimBlocks.find(uKey) != kVerbatimBlocks.end()) {
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
        throw std::runtime_error("Invalid schema for key: " + key);
      }
      if (keyInfo.isDeprecated()) {
        // Accepted so that older files still load, but never stored: the
        // key has no effect and must not be written back to a potential.
        reportDeprecatedKey(uKey, keyInfo.deprecated);
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

        // Perform nested parse
        tadah::core::CoreUserConfigParser parser;
        Context configCtx;
        try {
          configCtx = parser.parse(val);
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
} // end anonymous namespace

Context CoreUserConfigParser::parse(const std::string &filename) const {
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
  parseLines(lines, ctx, 1, filename);
  return ctx;
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
  parseLines(lines, ctx, 1, "");
  return ctx;
}

Context
CoreUserConfigParser::parse(const std::vector<std::string> &lines) const {
  ParseScope scope;

  Context ctx;
  parseLines(lines, ctx, 1, "");
  return ctx;
}

Context CoreUserConfigParser::parse(const std::vector<std::string> &lines,
                                    std::size_t first_line,
                                    const std::string &source) const {
  ParseScope scope;

  Context ctx;
  parseLines(lines, ctx, first_line, source);
  return ctx;
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

  // Provide examples if available
  if (!keyInfo.examples.empty()) {
    Logger::instance().log(LogLevel::ERROR, "  Examples:");
    for (const auto &ex : keyInfo.examples) {
      Logger::instance().log(LogLevel::ERROR, "    ", key, "   ", ex);
    }
  }

  // 4. Indicate failure
  return false;
}

bool CoreUserConfigParser::validateSchemaOnly(const std::string &text,
                                              const BlueprintKey &keyInfo)
{
  BlueprintValidator validator(keyInfo.value_type);
  return validator.validate(text);
}

} // namespace core
} // namespace tadah
