#include <tadah/core/logger.h>

/**
 * @brief Retrieves the singleton Logger instance.
 *
 * Provides a unified logger that is used throughout the project.
 * @return Reference to the Logger instance.
 */
Logger& Logger::instance() {
  static Logger logger_instance;
  return logger_instance;
}

/**
 * @brief Private constructor that initializes the Logger.
 *
 * Sets the default log level to ERROR and disables file logging.
 */
Logger::Logger() : currentLevel(LogLevel::ERROR), fileLoggingEnabled(false) {}

/**
 * @brief Sets the active logging level.
 *
 * Updates the verbosity filter which determines which messages are logged.
 * @param level The new logging level.
 */
void Logger::setLogLevel(LogLevel level) {
  std::lock_guard<std::mutex> lock(mutex_);
  currentLevel = level;
}

/**
 * @brief Retrieves the active logging level.
 *
 * Useful for inspecting or conditionally constructing log messages.
 * @return The currently active LogLevel.
 */
LogLevel Logger::getLogLevel() const {
  return currentLevel;
}

/**
 * @brief Configures file logging.
 *
 * Attempts to open the specified file for writing log messages.
 * When successful, subsequent log messages will be written to both console and file.
 * @param filename The name of the log file.
 * @return true if the file was successfully opened; false otherwise.
 */
bool Logger::setLogFile(const std::string &filename) {
  std::lock_guard<std::mutex> lock(mutex_);
  fileStream.open(filename.c_str(), std::ios::out | std::ios::app);
  fileLoggingEnabled = fileStream.is_open();
  return fileLoggingEnabled;
}

/**
 * @brief Converts a LogLevel to its string representation.
 *
 * Provides a human-readable string that is prepended to log messages.
 * @param level The log level to convert.
 * @return A string representing the log level.
 */
std::string Logger::logLevelToString(LogLevel level) const {
  switch (level) {
    case LogLevel::TRACE: return "TRACE";
    case LogLevel::DEBUG: return "DEBUG";
    case LogLevel::INFO:  return "INFO";
    case LogLevel::WARN:  return "WARN";
    case LogLevel::ERROR: return "ERROR";
    default:              return "LOG";
  }
}

/**
 * @brief Retrieves the current system time as a formatted string.
 *
 * Provides a timestamp for log messages.
 * @return A string representing the current time.
 */
std::string Logger::getCurrentTime() const {
  using namespace std::chrono;
  auto now = system_clock::now();
  std::time_t now_c = system_clock::to_time_t(now);
  std::string timeStr = std::ctime(&now_c);
  // Remove trailing newline from std::ctime.
  if (!timeStr.empty() && timeStr.back() == '\n')
    timeStr.pop_back();
  return timeStr;
}
