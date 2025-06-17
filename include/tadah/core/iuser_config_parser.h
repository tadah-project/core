#ifndef TADAH_CORE_IUSER_CONFIG_PARSER_H
#define TADAH_CORE_IUSER_CONFIG_PARSER_H

#include <string>
#include <vector>
#include <tadah/core/context.h>

namespace tadah {
namespace core {

/**
 * @brief Abstract interface for a user configuration parser.
 *
 * Defines methods to parse configuration (a keywords/values file) from various input sources.
 * The parsed key/value pairs are stored in a Context object.
 */
class IUserConfigParser {
public:
  virtual ~IUserConfigParser() {}

  /**
   * @brief Parses a configuration from a file.
   * @param filename The configuration filename.
   * @return A Context holding key/value pairs from the file.
   * @throws std::runtime_error on any parsing or validation error.
   */
  virtual Context parse(const std::string &filename) const = 0;

  /**
   * @brief Parses a configuration from an input stream.
   * @param in The input stream with configuration data.
   * @return A Context holding key/value pairs.
   * @throws std::runtime_error on any parsing or validation error.
   */
  virtual Context parse(std::istream &in) const = 0;

  /**
   * @brief Parses a configuration from a vector of lines.
   * @param lines The vector of strings representing configuration lines.
   * @return A Context holding key/value pairs.
   * @throws std::runtime_error on any parsing or validation error.
   */
  virtual Context parse(const std::vector<std::string>& lines) const = 0;
};

} // namespace core
} // namespace tadah

#endif // TADAH_CORE_IUSER_CONFIG_PARSER_H

