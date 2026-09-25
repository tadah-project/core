#ifndef TADAH_CORE_IUSER_CONFIG_PARSER_H
#define TADAH_CORE_IUSER_CONFIG_PARSER_H

#include <cstddef>
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

  /**
   * @brief Parses a configuration from lines taken out of a larger source.
   *
   * For a block of a file (a TASK section): @p lines[i] is line
   * @p first_line + i of @p source, so an error that names a line names the
   * line of the file, and the file.
   *
   * @param lines      The lines of the block, blank and comment lines
   *                   included.
   * @param first_line The line number (1-based) of @p lines[0] in @p source.
   * @param source     The name of the source (a file name), or empty when
   *                   it has none.
   * @return A Context holding key/value pairs.
   * @throws std::runtime_error on any parsing or validation error.
   */
  virtual Context parse(const std::vector<std::string>& lines,
                        std::size_t first_line,
                        const std::string& source) const = 0;
};

} // namespace core
} // namespace tadah

#endif // TADAH_CORE_IUSER_CONFIG_PARSER_H

