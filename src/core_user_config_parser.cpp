#include "tadah/core/blueprint_parser.h"
#include "tadah/core/blueprint_validator.h"
#include "tadah/core/core_user_config_parser.h"
#include "tadah/core/logger.h"
#include "tadah/core/utils.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

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
   * @brief Parse lines into an existing Context object.
   *
   * Circular dependency detection is handled by tracking CONFIG filenames
   * in configChain. When a CONFIG line is encountered, the file is pushed
   * into configChain, parsed, then popped after the nested parse.
   */
  void parseLines(const std::vector<std::string> &lines, Context &ctx) {
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

      // OPTIMIZER is a verbatim multi-line block, not a flat key. Slurp every
      // token between `OPTIMIZER` and the matching `ENDOPTIMIZER` into the
      // single (unbounded) Context key OPTIMIZER; the consuming engine (e.g.
      // `tadah refit` compact mode) parses that token stream itself. Mirrors
      // the CONFIG special-case below.
      if (uKey == "OPTIMIZER") {
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
          if (to_upper(bl) == "ENDOPTIMIZER") {
            closed = true;
            break;
          }
          body += bl + " ";
        }
        if (!closed) {
          throw std::runtime_error(
              "OPTIMIZER block is not closed with ENDOPTIMIZER");
        }
        i = k;  // resume after ENDOPTIMIZER
        if (!body.empty()) {
          ctx.add("OPTIMIZER", body);  // STRINGS key: add() splits on whitespace
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
      bool valid_schema = CoreUserConfigParser::validateWithBlueprint(key, val, keyMap.at(uKey));
      if (!valid_schema) {
        throw std::runtime_error("Invalid schema for key: " + key);
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
  // Check if this is a top-level parse
  if (parseDepth == 0) {
    // Clear the chain before starting a fresh top-level parse
    configChain.clear();
  }
  ++parseDepth; // Increase recursion depth

  std::ifstream infile(filename.c_str());
  if (!infile.is_open()) {
    Logger::instance().log(
        LogLevel::ERROR,
        "CoreUserConfigParser::parse -> Failed to open file: ", filename);
    --parseDepth; 
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
  parseLines(lines, ctx);

  --parseDepth; // Decrease recursion depth
  // If returning to top-level, reset chain
  if (parseDepth == 0) {
    configChain.clear();
  }

  return ctx;
}

Context CoreUserConfigParser::parse(std::istream &in) const {
  // Check if this is a top-level parse
  if (parseDepth == 0) {
    configChain.clear();
  }
  ++parseDepth;

  std::vector<std::string> lines;
  {
    std::string line;
    while (std::getline(in, line)) {
      lines.push_back(line);
    }
  }

  Context ctx;
  parseLines(lines, ctx);

  --parseDepth;
  if (parseDepth == 0) {
    configChain.clear();
  }

  return ctx;
}

Context
CoreUserConfigParser::parse(const std::vector<std::string> &lines) const {
  if (parseDepth == 0) {
    configChain.clear();
  }
  ++parseDepth;

  Context ctx;
  parseLines(lines, ctx);

  --parseDepth;
  if (parseDepth == 0) {
    configChain.clear();
  }

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
  // 1. Build validator from keyInfo schemas
  BlueprintValidator validator(keyInfo.value_type);
  
  // 2. Attempt validation
  if (validator.validate(text)) {
    return true;
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

} // namespace core
} // namespace tadah
