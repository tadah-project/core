#include "tadah/core/blueprint_parser.h"
#include "tadah/core/blueprint_validator.h"
#include "tadah/core/core_user_config_parser.h"
#include "tadah/core/logger.h"
#include "tadah/core/utils.h"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace tadah {
namespace core {

namespace {
/**
 * @brief Parse lines into an existing Context object.
 *
 * Ignores blank lines and comment lines starting with '#'.
 * For a valid line, the first token is treated as key, and all subsequent
 * tokens are appended as values. In-line comments (beginning with '#') are also
 * handled by stopping further parsing on that line.
 *
 * Each line is validated against the blueprint schema.
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
    const std::vector<std::string> &schemas = keyMap.at(uKey).value_type;
    BlueprintValidator validator(schemas);
    std::string val = removeFirstWord(rawLine, key);
    bool valid_schema = validator.validate(val);
    if (!valid_schema) {
      Logger::instance().log(
          LogLevel::ERROR,
          "CoreUserConfigParser::parse -> Invalid schema for key:value: ",
          key, ":", val);
      // log allowed schemas
      Logger::instance().log(LogLevel::ERROR, "  Allowed schemas:");
      for (const auto &schema : schemas) {
        Logger::instance().log(LogLevel::ERROR, "    ", schema);
      }
      // log the user friendlier format
      const std::vector<std::string> &format = keyMap.at(uKey).value_format;
      Logger::instance().log(LogLevel::ERROR, "  Format:");
      for (const auto &fmt : format) {
        Logger::instance().log(LogLevel::ERROR, "    ", fmt);
      }
      // finally provide examples if available
      const auto &examples = keyMap.at(uKey).examples;
      if (!examples.empty()) {
        Logger::instance().log(LogLevel::ERROR, "  Examples:");
        for (const auto &ex : examples) {
          Logger::instance().log(LogLevel::ERROR, "    ", ex);
        }
      }
      
      throw std::runtime_error("Invalid schema for key: " + key);
    }
    // the rest are values
    for (size_t j = 1; j < tokens.size(); ++j) {
      ctx.add(key, tokens[j]);
    }
  }
}
} // end anonymous namespace

Context CoreUserConfigParser::parse(const std::string &filename) const {
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

  // Construct a local Context, parse lines into it
  Context ctx;
  parseLines(lines, ctx);
  return ctx; // Named object -> compiler can apply (N)RVO in C++11
}

Context CoreUserConfigParser::parse(std::istream &in) const {
  std::vector<std::string> lines;
  {
    std::string line;
    while (std::getline(in, line)) {
      lines.push_back(line);
    }
  }

  Context ctx;
  parseLines(lines, ctx);
  return ctx;
}

Context
CoreUserConfigParser::parse(const std::vector<std::string> &lines) const {
  Context ctx;
  parseLines(lines, ctx);
  return ctx;
}

} // namespace core
} // namespace tadah
