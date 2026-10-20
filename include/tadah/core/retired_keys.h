#ifndef TADAH_CORE_RETIRED_KEYS_H
#define TADAH_CORE_RETIRED_KEYS_H

#include <tadah/core/blueprint_parser.h>
#include <tadah/core/context.h>

#include <string>
#include <vector>

namespace tadah {
namespace core {

/** @file retired_keys.h
 *  The texts of the retired keys (blueprint field `retired`, RetiredKey):
 *  the notice of a parse that read some, the refusal of a fit whose
 *  configuration records some, and the text of one key.
 *
 *  A file that has a retired key line still parses: CoreUserConfigParser
 *  records the line on the Context it returns (Context::retired_keys())
 *  and gives one notice for the parse (retired_keys_notice()), so every
 *  load of a potential that carries them works. A fit refuses such a
 *  Context with the conversion (require_no_retired_keys()), whatever read
 *  it: tadah train, tadah hpo and a program's own training. */

/** "<KEY> is a retired key: <reason> <replacement>. <conversion>": the
 *  text of the retired key @p key, upper case, whose blueprint entry is
 *  @p info (Context::add() and a read of it refuse it with this text;
 *  tadah explain and the command line give it). */
std::string retired_key_text(const std::string &key, const BlueprintKey &info);

/** The notice of a parse that read the retired lines @p lines (at least
 *  one): "Retired keys EWEIGHT (line 2 of old.tadah) and FWEIGHT (line 3
 *  of old.tadah) are ignored: ..." (one key: "Retired key EWEIGHT (line 2
 *  of old.tadah) is ignored: ..."), saying that they change nothing a
 *  prediction gives and that a fit refuses them, and where the conversion
 *  is. A line without a file is named "line N". Without the "WARNING: "
 *  that report_key_notice() adds. */
std::string retired_keys_notice(const std::vector<RetiredKeyLine> &lines);

/** The refusal of a fit whose configuration records the retired lines
 *  @p lines (at least one): "<who>: the configuration has keys retired in
 *  Tadah! 1.6.0, and a fit refuses them: EWEIGHT 1.0 (line 2 of
 *  old.tadah), FWEIGHT 100 (line 3 of old.tadah). EWEIGHT: <conversion>
 *  FWEIGHT: <conversion> ...", each key's conversion once, in the order
 *  of its first line, then what to do with the lines. A line that a
 *  deletion converts says so: WDBFILE_AUTO 0 ("(line 6 of old.tadah; 0:
 *  converted by deleting the line)") and, given the configuration
 *  @p context, FWEIGHT with FORCE false and SWEIGHT with STRESS false
 *  ("(line 3 of old.tadah; FORCE false here: converted by deleting the
 *  line)"). */
std::string retired_keys_refusal(const std::vector<RetiredKeyLine> &lines,
                                 const std::string &who,
                                 const Context *context = nullptr);

/** Refuses a fit of @p context when it records a retired line
 *  (Context::retired_keys()), with retired_keys_refusal(lines, @p who,
 *  &@p context).
 *  Every fit calls it before it reads anything (M_BLR and M_KRR train,
 *  tadah train, tadah hpo when a run starts).
 *  @throws std::runtime_error */
void require_no_retired_keys(const Context &context, const std::string &who);

}  // namespace core
}  // namespace tadah

#endif  // TADAH_CORE_RETIRED_KEYS_H
