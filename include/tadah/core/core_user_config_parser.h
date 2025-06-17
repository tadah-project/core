#ifndef TADAH_CORE_CORE_USER_CONFIG_PARSER_H
#define TADAH_CORE_CORE_USER_CONFIG_PARSER_H

#include <tadah/core/iuser_config_parser.h>

namespace tadah {
namespace core {

/**
 * @brief Default “dumb” parser that reads key/value pairs from configuration input.
 *
 * This implementation assumes that the configuration file contains one key/value pair per line
 * (ignoring blank lines and comments). It validates every key against the allowed keywords
 * provided by the blueprint (via Context). No structural grouping (global vs. task) is performed.
 */
class CoreUserConfigParser : public IUserConfigParser {
public:
  CoreUserConfigParser() = default;
  virtual ~CoreUserConfigParser() = default;

  /**
   * @brief Parses configuration from a file.
   * @param filename The configuration file path.
   * @return A Context containing all key/value pairs.
   * @throws std::runtime_error if any key is not allowed or a parsing error occurs.
   */
  Context parse(const std::string &filename) const override;

  /**
   * @brief Parses configuration from an input stream.
   * @param in The input stream.
   * @return A Context containing all key/value pairs.
   * @throws std::runtime_error if any key is not allowed or a parsing error occurs.
   */
  Context parse(std::istream &in) const override;

  /**
   * @brief Parses configuration from a vector of strings.
   * @param lines The vector of configuration lines.
   * @return A Context containing all key/value pairs.
   * @throws std::runtime_error if any key is not allowed or a parsing error occurs.
   */
  Context parse(const std::vector<std::string>& lines) const override;

  /**
   * @brief Validate a (key, value) pair against Blueprint schema.
   *
   * Logs any errors if the value does not comply with the schema for this key
   * (including showing allowed schemas, formats, and examples).
   *
   * @param key         The user-supplied key.
   * @param val         The raw, trimmed value string to validate.
   * @param keyInfo     The BlueprintKey struct containing the schema,
   *                    formats, and examples for this key.
   * @return            True if validation passes, otherwise false (and logs why).
   */
  static bool validateWithBlueprint(
      const std::string &key,
      const std::string &val,
      const BlueprintKey &keyInfo);

  /**
   * @brief Validates a vector of string values against the blueprint.
   *  
   *  The same logic as validateWithBlueprint, but for a vector of values.
   */
  static bool validateWithBlueprint(
      const std::string &key,
      const std::vector<std::string> &vals,
      const BlueprintKey &keyInfo);

private:
  /**
   * @brief Internal helper that actually calls the BlueprintValidator and logs on failure.
   *
   * @param key        The user’s key (for logging)
   * @param text       The concatenated or original text to validate
   * @param keyInfo    The blueprint specification (schema, format, examples, etc.)
   * @return           true if validation passes, false otherwise (and logs).
   */
  static bool validateWithBlueprintImpl(
      const std::string &key,
      const std::string &text,
      const BlueprintKey &keyInfo);

};
} // namespace core
} // namespace tadah

#endif // TADAH_CORE_CORE_USER_CONFIG_PARSER_H

