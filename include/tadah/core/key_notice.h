#ifndef TADAH_CORE_KEY_NOTICE_H
#define TADAH_CORE_KEY_NOTICE_H

#include <string>

namespace tadah {
namespace core {

/**
 * @brief Writes the notice for a key that is accepted but has no effect:
 *        `WARNING: <subject> is ignored: <reason>` and a newline, to
 *        standard error.
 *
 * The notice every such key gets, once per key and process through
 * report_ignored_key_once(): a deprecated key (CoreUserConfigParser,
 * subject `Deprecated key MBLOCK`) and a key that has no effect on the
 * potential that carries it (EMBF0 or RHOFORM without a DM_REAM block,
 * subject `Key EMBF0`).
 *
 * Deliberately not a Logger message: the notice must reach every user of
 * the file whatever the Logger level, including pair_style tadah in LAMMPS
 * (which leaves the Logger at ERROR) and a file whose VERBOSE 0 line
 * lowered the level. The line is composed first and written with one
 * insertion; std::cerr is unit-buffered, so it is flushed at once.
 */
void report_ignored_key(const std::string &subject, const std::string &reason);

/**
 * @brief report_ignored_key(@p subject, @p reason), at most once per
 *        process for @p key.
 *
 * The first call for @p key in the process writes the notice and returns
 * true; every later call for the same @p key (any thread, any subject or
 * reason) writes nothing and returns false. Thread-safe.
 */
bool report_ignored_key_once(const std::string &key, const std::string &subject,
                             const std::string &reason);

/**
 * @brief Whether report_ignored_key_once() has written the notice for
 *        @p key in this process.
 */
bool ignored_key_reported(const std::string &key);

} // namespace core
} // namespace tadah

#endif // TADAH_CORE_KEY_NOTICE_H
