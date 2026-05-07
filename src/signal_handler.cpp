#include <tadah/core/signal_handler.h>

#include <csignal>
#include <cstring>

namespace tadah {
namespace core {

namespace {

volatile std::sig_atomic_t g_stop_flag    = 0;
volatile std::sig_atomic_t g_signal_seen  = 0;
bool                       g_installed    = false;

/* Async-signal-safe handler. Avoid any non-trivial calls here. */
extern "C" void tadah_signal_handler(int signo) noexcept
{
    /* Only record the first signal — later signals don't downgrade the
     * reason text we'll show the user. */
    if (g_signal_seen == 0) g_signal_seen = signo;
    g_stop_flag = 1;
}

}   // namespace

void SignalHandler::install()
{
    if (g_installed) return;

    struct sigaction sa{};
    sa.sa_handler = &tadah_signal_handler;
    sigemptyset(&sa.sa_mask);
    /* SA_RESTART so a slow read()/getline() doesn't error out with
     * EINTR; the per-eval poll picks the flag up at the next safe
     * point. */
    sa.sa_flags = SA_RESTART;

    /* Install for the three signals we care about. Failures are
     * non-fatal — the flag-based polling falls through to "no signal
     * seen" which is the expected behaviour without a handler. */
    ::sigaction(SIGINT,  &sa, nullptr);
    ::sigaction(SIGTERM, &sa, nullptr);
    ::sigaction(SIGUSR2, &sa, nullptr);

    g_installed = true;
}

bool SignalHandler::stop_requested()
{
    return g_stop_flag != 0;
}

int SignalHandler::signal_seen()
{
    return static_cast<int>(g_signal_seen);
}

std::string SignalHandler::reason()
{
    int s = static_cast<int>(g_signal_seen);
    switch (s) {
        case 0:        return "no signal";
        case SIGINT:   return "SIGINT";
        case SIGTERM:  return "SIGTERM";
        case SIGUSR2:  return "SIGUSR2";
        default:       return std::string("signal ") + std::to_string(s);
    }
}

}   // namespace core
}   // namespace tadah
