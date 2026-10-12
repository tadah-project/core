#ifndef TADAH_CORE_CORE_USER_CONFIG_PARSER_H
#define TADAH_CORE_CORE_USER_CONFIG_PARSER_H

#include <tadah/core/iuser_config_parser.h>

#include <map>
#include <string>

namespace tadah {
namespace core {

/**
 * @brief Default “dumb” parser that reads key/value pairs from configuration input.
 *
 * This implementation assumes that the configuration file contains one key/value pair per line
 * (ignoring blank lines and comments). It validates every key against the allowed keywords
 * provided by the blueprint (via Context). No structural grouping (global vs. task) is performed.
 *
 * A key the blueprint marks as deprecated (BlueprintKey::deprecated) is accepted so that older
 * configuration and potential files still load: its value is validated like any other, a
 * notice naming the key and the reason (`WARNING: Deprecated key MBLOCK is ignored: ...`) is
 * written to standard error once per key and process (report_ignored_key_once(), key_notice.h:
 * however many lines, CONFIG files or parses read it), and the key is not stored in the
 * returned Context. The notice bypasses the Logger, so neither the Logger
 * level nor VERBOSE suppresses it: pair_style tadah in LAMMPS, which leaves the Logger at
 * ERROR, shows it too.
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
   * @brief Parses configuration lines that start at line @p first_line of
   *        @p source (a block of a file, such as a TASK section).
   *
   * An error that names a line names the line of @p source ("on line 6 of
   * run.task"), not its index in @p lines.
   * @throws std::runtime_error if any key is not allowed or a parsing error occurs.
   */
  Context parse(const std::vector<std::string>& lines, std::size_t first_line,
                const std::string& source) const override;

  /**
   * @brief Refuses the keys of @p reasons (key -> why) in everything this
   *        parser reads.
   *
   * A line that sets one of them, in any letter case, is an error at that
   * line: "Key FORCE on line 2 of val.tadah is not accepted here: <reason>".
   * A file included with CONFIG is read under the same refusal, so its
   * error names the included file and its line. A later call replaces the
   * set; an empty map refuses nothing (the default).
   *
   * For a file whose role leaves no meaning to a key that is valid
   * elsewhere: HPO's --validation file refuses FORCE and STRESS, since what
   * validation predicts follows PC_FRMSE and PC_SRMSE.
   */
  void refuse_keys(std::map<std::string, std::string> reasons);

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

  /**
   * @brief Whether @p text matches the key's value_type schema (the
   *        BlueprintValidator), without the allowed_values check; no log.
   */
  static bool validateSchemaOnly(const std::string &text,
                                 const BlueprintKey &keyInfo);

  /**
   * @brief The separators that join the values of @p text where only
   *        blanks separate values: "','", "'+'" or "',' and '+'", or ""
   *        when there are none.
   *
   * A configuration line (and a command-line option) is split into values
   * at blanks only, and its key's types take a comma or a '+' between two
   * values nowhere but inside a STRING word and between the entries of an
   * index list. For a @p text that fails the key's types, this returns the
   * separator(s) whose replacement by blanks makes it a value of the key
   * (types and allowed values): every comma, every '+' joined to the
   * character before it (not after a blank, a comma or an exponent's e/E),
   * or both, tried in that order; @p respaced is then that text with its
   * words joined by one blank ("0.1,0.2" gives "','" and "0.1 0.2"). An
   * empty result (and @p respaced untouched) when @p text holds neither, or
   * when blanks would not make it valid either.
   */
  static std::string misusedSeparators(const std::string &text,
                                       const BlueprintKey &keyInfo,
                                       std::string &respaced);

private:
  /**
   * @brief Internal helper that actually calls the BlueprintValidator and logs on failure.
   *
   * @param key        The user’s key (for logging)
   * @param text       The concatenated or original text to validate
   * @param keyInfo    The blueprint specification (schema, format, examples, etc.)
   * @return           true if validation passes, false otherwise (and logs).
   *                   A value outside the key's allowed_values fails too.
   */
  static bool validateWithBlueprintImpl(
      const std::string &key,
      const std::string &text,
      const BlueprintKey &keyInfo);

  /// Upper-case key -> the reason it is refused (refuse_keys()).
  std::map<std::string, std::string> refused_;
};
} // namespace core
} // namespace tadah

#endif // TADAH_CORE_CORE_USER_CONFIG_PARSER_H

