#ifndef TADAH_CORE_SIGNAL_HANDLER_H
#define TADAH_CORE_SIGNAL_HANDLER_H

/*
 *  SignalHandler — async-signal-safe stop flag for long-running loops.
 *
 *  install() registers handlers for SIGINT, SIGTERM, and SIGUSR2:
 *    - SIGINT  : ctrl+C from interactive shells.
 *    - SIGTERM : default `kill` and SLURM cancellation.
 *    - SIGUSR2 : SLURM advance warning when configured with
 *                `--signal=B:USR2@<seconds>`.
 *
 *  The handler itself is async-signal-safe — it only writes a
 *  std::sig_atomic_t flag and stores the signal number. All "what
 *  happened" reasoning (reason text, dispatch to participants) runs in
 *  normal context, polled by long-running loops via stop_requested().
 *
 *  Once raised, the flag is never lowered. Re-entering main() in a
 *  fresh process is the only way to reset, which is intentional —
 *  partially-honored shutdowns are confusing.
 */

#include <string>

namespace tadah {
namespace core {

class SignalHandler {
public:
    /* Install handlers for SIGINT/SIGTERM/SIGUSR2. Idempotent — repeat
     * calls are no-ops. Keeps any previous SIGCHLD/SIGPIPE/etc.
     * handlers untouched. */
    static void install();

    /* True once any of the watched signals has been delivered. Polled
     * by the HPO per-eval loop and BasinsPolicy stage boundaries. */
    static bool stop_requested();

    /* Numeric signal that triggered the stop (0 if none yet). */
    static int  signal_seen();

    /* Human-readable description ("SIGINT", "SIGTERM", "SIGUSR2",
     * or "no signal"). Safe to call any time. */
    static std::string reason();

private:
    SignalHandler() = delete;
};

}   // namespace core
}   // namespace tadah

#endif
