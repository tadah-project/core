#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <sstream>
#include <mutex>
#include <string>
#include <chrono>
#include <ctime>
#include <fstream>

#ifdef MPI_ENABLED
  #include <mpi.h>
#endif

/**
 * @brief Enumerates severity levels for logging.
 *
 * Lower numeric values output only the most critical messages.
 * Increasing values enable more verbose output.
 */
enum class LogLevel {
  ERROR = 0,  ///< Critical error messages only.
  WARN  = 1,  ///< Warning messages.
  INFO  = 2,  ///< Basic informational messages.
  DEBUG = 3,  ///< Debug messages for development.
  TRACE = 4   ///< Detailed trace messages for maximum verbosity.
};

/**
 * @brief Logger class for centralized, thread-safe logging.
 *
 * Implements a singleton pattern to provide a unified logging facility.
 * Supports writing log messages to both console and file.
 * Optionally includes MPI rank information when MPI builds are used.
 *
 * Usage Example:
 * @code
 * // Include the logger header in your source file.
 * #include "Logger.hpp"
 *
 * int main(int argc, char* argv[]) {
 *   // Set logging level to INFO.
 *   Logger::instance().setLogLevel(LogLevel::INFO);
 *
 *   // Optionally, configure file logging.
 *   if (!Logger::instance().setLogFile("app.log")) {
 *     std::cerr << "Could not open log file." << std::endl;
 *   }
 *
 *   // Log an informational message.
 *   Logger::instance().log(LogLevel::INFO, "Application started.");
 *
 *   // Log a debug message: may be compiled out in release builds.
 *   Logger::instance().log(LogLevel::DEBUG, "Debug details: x = ", 42);
 *
 *   // Application code...
 *
 *   return 0;
 * }
 * @endcode
 */
class Logger {
public:
  /**
   * @brief Retrieves the singleton logger instance.
   *
   * Ensures that only one logging object is used throughout the project.
   * @return Reference to the Logger instance.
   */
  static Logger& instance();

  /**
   * @brief Sets the active logging level.
   *
   * Configures which log messages pass the verbosity filter.
   * @param level The logging level to use.
   */
  void setLogLevel(LogLevel level);

  /**
   * @brief Retrieves the current logging level.
   *
   * @return The active log level.
   */
  LogLevel getLogLevel() const;

  /**
   * @brief Configures file logging.
   *
   * Opens the given file name to output log messages. Once set,
   * log messages are output both to console and the file.
   *
   * @param filename The file name to write log messages.
   * @return true if file logging is enabled successfully, false otherwise.
   */
  bool setLogFile(const std::string &filename);

  /**
   * @brief Logs a message with a selected severity level.
   *
   * Uses variadic templates and recursive helper functions to enable flexible and efficient
   * message construction. If the message level is higher than the active level, the message is
   * not constructed.
   *
   * In non-debug builds (when DEBUG_BUILD is not defined), DEBUG and TRACE logs are compiled out.
   *
   * @tparam Args Variadic template parameters.
   * @param messageLevel The severity level for the message.
   * @param args Parts of the message to be concatenated.
   */
  template <typename... Args>
  void log(LogLevel messageLevel, Args&&... args) {
    // In release build, skip DEBUG and TRACE messages.
    #if !defined(DEBUG_BUILD)
      if (messageLevel == LogLevel::DEBUG || messageLevel == LogLevel::TRACE)
        return;
    #endif

    if (messageLevel > currentLevel)
      return;  // Skip message construction if level is not enabled.

    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    // Append a timestamp for traceability.
    oss << "[" << getCurrentTime() << "] ";
#ifdef MPI_ENABLED
    // Append MPI rank if running under MPI.
    int worldRank = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    oss << "Rank " << worldRank << " ";
#endif
    // Append human-readable log level.
    oss << logLevelToString(messageLevel) << ": ";
    // Append all message parts using recursive helper.
    appendToStream(oss, std::forward<Args>(args)...);

    const std::string finalMessage = oss.str();
    std::cout << finalMessage << "\n";  // Output to console.

    // If file logging is enabled, write the message to file.
    if (fileLoggingEnabled && fileStream.is_open()) {
      fileStream << finalMessage << "\n";
      fileStream.flush();
    }
  }

private:
  /**
   * @brief Private constructor to enforce the singleton pattern.
   *
   * Initializes the log level to ERROR by default to avoid extraneous output.
   */
  Logger();

  /**
   * @brief Converts a LogLevel enum value to its string representation.
   *
   * Improves clarity by outputting a human-readable log level.
   * @param level The log level to convert.
   * @return A string representation of the log level.
   */
  std::string logLevelToString(LogLevel level) const;

  /**
   * @brief Retrieves the current system time as a formatted string.
   *
   * Provides a timestamp for each log message to aid debug and audit trails.
   * @return The current time formatted as a string.
   */
  std::string getCurrentTime() const;

  /**
   * @brief Base case for the recursive variadic template helper.
   *
   * Terminates the recursion when no arguments remain.
   *
   * @param oss Output string stream.
   */
  void appendToStream(std::ostringstream &oss) {}

  /**
   * @brief Recursive helper function to stream multiple values.
   *
   * Processes the first argument and recurses to process the remaining arguments.
   *
   * @tparam T First argument type.
   * @tparam Rest Remaining argument types.
   * @param oss Output string stream.
   * @param first The first argument to stream.
   * @param rest Remaining arguments.
   */
  template <typename T, typename... Rest>
  void appendToStream(std::ostringstream &oss, T&& first, Rest&&... rest) {
    oss << std::forward<T>(first);
    appendToStream(oss, std::forward<Rest>(rest)...);
  }

  LogLevel currentLevel;       ///< The active logging level.
  mutable std::mutex mutex_;   ///< Protects log writes for thread-safety.

  bool fileLoggingEnabled;     ///< Flag indicating if file logging is enabled.
  std::ofstream fileStream;    ///< Output file stream for log messages.
};

#endif // LOGGER_H
