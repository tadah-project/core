#ifndef TADAH_CORE_IUSER_CONFIG_PARSER_H
#define TADAH_CORE_IUSER_CONFIG_PARSER_H

#include <cstddef>
#include <functional>
#include <set>
#include <string>
#include <vector>
#include <tadah/core/context.h>

namespace tadah {
namespace core {

/**
 * @brief A key line that a parse read and did not store, because its
 *        KeyLineRule selected it.
 */
struct SkippedKeyLine {
  /** The file the line is in: as given to the parse, or as the CONFIG line
   *  that includes it names it; empty for lines without a source. */
  std::string source;
  /** Its number (1-based) in @c source. */
  std::size_t line = 0;
  /** The key, upper case. */
  std::string key;
  /** Its values, as written. */
  std::vector<std::string> values;
};

/**
 * @brief Whether a parse skips the line of @p key (upper case) of a file
 *        whose key lines give the keys @p file_keys (upper case).
 *
 * @p file_keys are the keys of the lines of the file (or of the block of a
 * file, such as a TASK section) that holds the line, a CONFIG line and the
 * name of a block (INIT, OPTIMIZER, ...) among them: not the words inside a
 * block, and not the keys of a file it includes with CONFIG, which is
 * judged by its own lines.
 */
using KeyLineRule = std::function<bool(const std::set<std::string> &file_keys,
                                       const std::string &key)>;

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

  /**
   * @brief parse(lines, first_line, source) that skips the key lines
   *        @p skip selects.
   *
   * Every line is read and checked as parse(lines, first_line, source)
   * reads it: an unknown key, an invalid value or a refused key is an error
   * at its line. A key line (not a CONFIG line, not a block) for which
   * @p skip(file_keys, key) is true is then not stored: it is appended to
   * @p skipped instead, in the order read. A file a CONFIG line includes is
   * read under the same rule, with the keys of its own lines.
   *
   * @param lines      The lines of the block, blank and comment lines
   *                   included.
   * @param first_line The line number (1-based) of @p lines[0] in @p source.
   * @param source     The name of the source (a file name), or empty when
   *                   it has none.
   * @param skip       The rule (KeyLineRule).
   * @param skipped    The lines not stored are appended to it.
   * @return A Context holding the key/value pairs stored.
   * @throws std::runtime_error on any parsing or validation error.
   */
  virtual Context parse(const std::vector<std::string>& lines,
                        std::size_t first_line,
                        const std::string& source,
                        const KeyLineRule& skip,
                        std::vector<SkippedKeyLine>& skipped) const = 0;
};

} // namespace core
} // namespace tadah

#endif // TADAH_CORE_IUSER_CONFIG_PARSER_H

