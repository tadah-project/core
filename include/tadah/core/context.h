/**
 * @file context.h
 * @brief Thread-safe configuration context respecting blueprint defaults.
 *
 * The Context class stores uppercase keys, each mapped to a vector of strings.
 * Keys are validated against a blueprint, which also may provide defaultValue tokens.
 * If a requested key doesn't exist, the blueprint's defaultValue is inserted (if available).
 *
 * This class is move-only:
 *  - Copy constructor/assignment are deleted because of std::mutex and a reference member.
 *  - A custom move constructor/assignment ensures objects can be returned by value.
 */

#ifndef TADAH_CORE_CONTEXT_H
#define TADAH_CORE_CONTEXT_H

#include <map>
#include <string>
#include <vector>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <algorithm>

#include <tadah/core/logger.h>
#include <tadah/core/blueprint_parser.h>

/**
 * @namespace tadah
 * Main project namespace.
 */
namespace tadah {

/**
 * @namespace core
 * Core implementation details of the tadah project.
 */
namespace core {

/**
 * @class Context
 * @brief Thread-safe key-value container that consults the blueprint for allowed keys and defaults.
 *
 * - **Case-insensitive** keys are stored as uppercase.
 * - If a key is absent on get(), a default from the blueprint is inserted if available.
 * - The blueprint also enforces a maximum capacity (|value_N[0]|).
 * - Due to storing a std::mutex and a reference to the blueprint, it is **move-only**.
 */
class Context {
public:
  /**
   * @brief Constructs an empty Context referencing the static blueprint map.
   */
  Context();

  /**
   * @brief Move constructor.
   * Transfers ownership of storage_ while retaining the same blueprint reference.
   */
  Context(Context&& other) noexcept;

  /**
   * @brief Move assignment.
   * Transfers ownership of storage_ while retaining the same blueprint reference.
   */
  Context& operator=(Context&& other) noexcept;

  // Copy is disallowed due to mutex and reference members.
  Context(const Context&) = delete;
  Context& operator=(const Context&) = delete;

  ~Context() = default;

  /**
   * @brief Returns all values for a key.
   *
   * If absent, tries inserting the blueprint's default.
   * If still missing, throws std::runtime_error.
   *
   * @param key Key to retrieve (case-insensitive).
   * @return const reference to the stored vector of string values.
   */
  const std::vector<std::string>& operator()(const std::string &key) const;

  /**
   * @brief Returns the first value for the key, converted to type T.
   *
   * If absent, attempts to insert a default from the blueprint.  
   * Throws std::runtime_error on failure or if conversion fails.
   *
   * @tparam T Destination type (e.g., int, double, bool, etc.).
   * @param key Key name.
   * @return The converted value.
   */
  template <typename T>
  T get(const std::string &key) const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::string upKey = uppercase(key);

    auto it = storage_.find(upKey);
    if (it == storage_.end() || it->second.empty()) {
      attemptDefaultInsertion_NoLock(upKey);
      it = storage_.find(upKey);
      if (it == storage_.end() || it->second.empty()) {
        Logger::instance().log(LogLevel::ERROR, "Context::get -> Key not found: ", upKey);
        throw std::runtime_error("Key not found (no default): " + upKey);
      }
    }
    return convertStringTo<T>(it->second[0], upKey);
  }

  /**
   * @brief Returns the n-th value for the key, converted to type T.
   *
   * If absent, attempts to insert a blueprint default, then checks range.
   * Throws std::runtime_error if still missing, or if n is out-of-range, or conversion fails.
   *
   * @tparam T Destination type.
   * @param key Key name.
   * @param n Index in the values vector.
   * @return The converted value.
   */
  template <typename T>
  T get(const std::string &key, size_t n) const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::string upKey = uppercase(key);

    auto it = storage_.find(upKey);
    if (it == storage_.end() || it->second.empty()) {
      attemptDefaultInsertion_NoLock(upKey);
      it = storage_.find(upKey);
      if (it == storage_.end() || it->second.empty()) {
        Logger::instance().log(LogLevel::ERROR, "Context::get(n) -> Key not found: ", upKey);
        throw std::runtime_error("Key not found (no default): " + upKey);
      }
    }
    if (n >= it->second.size()) {
      Logger::instance().log(LogLevel::ERROR, "Context::get(n) -> Index out of range: ", n);
      throw std::runtime_error("Index out of range for key: " + upKey);
    }
    return convertStringTo<T>(it->second[n], upKey);
  }

  /**
   * @brief Fills a provided container with values for the key.
   *
   * If the key is absent, attempts to insert the blueprint default.
   * Logs a warning if there are more stored values than the container size.
   *
   * @tparam Container type with operator[] and size() (e.g., std::vector<T>).
   * @param key Key name.
   * @param out The container to fill.
   * @throws std::runtime_error if the key is missing or conversion fails.
   */
  template <typename Container>
  void get(const std::string &key, Container &out) const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::string upKey = uppercase(key);

    auto it = storage_.find(upKey);
    if (it == storage_.end() || it->second.empty()) {
      attemptDefaultInsertion_NoLock(upKey);
      it = storage_.find(upKey);
      if (it == storage_.end() || it->second.empty()) {
        Logger::instance().log(LogLevel::ERROR, "Context::get(container) -> Key not found: ", upKey);
        throw std::runtime_error("Key not found (no default): " + upKey);
      }
    }
    const std::vector<std::string>& vals = it->second;
    if (vals.size() > out.size()) {
      Logger::instance().log(LogLevel::WARNING,
        "Context::get(container) -> More values (", vals.size(), ") than container capacity (", out.size(), ")");
    }
    for (size_t i = 0; i < vals.size() && i < out.size(); ++i) {
      out[i] = convertStringTo<typename Container::value_type>(vals[i], upKey);
    }
  }

  /**
   * @brief Appends a value to an existing key, respecting blueprint capacity.
   *
   * If the key is not allowed by blueprint, or if capacity is reached, throws std::runtime_error.
   *
   * @tparam T Type convertible to string with operator<<.
   * @param key Key name.
   * @param val Value to append.
   */
  template <typename T>
  void add(const std::string &key, const T &val) {
    std::string upKey = uppercase(key);
    // Validate blueprint
    auto itBP = allowedKeys_.find(upKey);
    if (itBP == allowedKeys_.end()) {
      Logger::instance().log(LogLevel::ERROR, "Context::add -> Key not allowed: ", upKey);
      throw std::runtime_error("Key not allowed: " + upKey);
    }
    const BlueprintItem &bi = itBP->second;
    if (bi.value_N.empty()) {
      Logger::instance().log(LogLevel::ERROR, "Context::add -> blueprint missing value_N for: ", upKey);
      throw std::runtime_error("Blueprint incomplete for key: " + upKey);
    }
    size_t maxVals = static_cast<size_t>( std::abs(bi.value_N[0]) );

    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<std::string> &bucket = storage_[upKey];
    if (bucket.size() >= maxVals) {
      Logger::instance().log(LogLevel::ERROR, "Context::add -> capacity exceeded for: ", upKey);
      throw std::runtime_error("Exceeded capacity for key: " + upKey);
    }
    std::ostringstream oss;
    oss << std::boolalpha << val;
    bucket.push_back(oss.str());
  }

  /**
   * @brief Removes all values for a given key.
   * @param key The key name.
   * @return 1 if the key existed and was removed, 0 otherwise.
   */
  size_t remove(const std::string &key);

  /**
   * @brief Clears all key-value pairs.
   */
  void clear();

  /**
   * @brief Returns the number of values stored for a key.
   *
   * If the key doesn't exist, attempts blueprint default insertion.
   * Throws std::runtime_error if still missing afterwards.
   *
   * @param key The key name.
   * @return Number of values for that key.
   */
  size_t size(const std::string &key) const;

  /**
   * @brief Returns the number of distinct keys currently stored.
   */
  size_t size() const;

  /**
   * @brief Checks if the context contains a given key.
   *
   * If absent, tries to insert a default from blueprint if available.
   * This means calling contains() can cause side effects.
   *
   * @param key The key name.
   * @return True if eventually found or default inserted, false otherwise.
   */
  bool contains(const std::string &key) const;

  /**
   * @brief Serializes all data into a vector of chars (plaintext).
   *
   * Format is lines of "KEY value1 value2 ...".
   */
  std::vector<char> serialize() const;

  /**
   * @brief Deserializes from a vector of chars (plaintext). Clears existing data first.
   *
   * Each line is "KEY value1 value2 ...". Keys must be allowed by blueprint. 
   * Over-capacity triggers an error.
   *
   * @param buffer The serialized data.
   * @throws std::runtime_error on invalid key or capacity exceed.
   */
  void deserialize(const std::vector<char> &buffer);

private:
  /**
   * @brief Attempts to insert the blueprint's defaultValue for upKey if it exists and is not empty.
   *
   * This method modifies storage_, so it must only be called when mtx_ is locked.
   * Marking storage_ as mutable allows this logic in const methods.
   *
   * @param upKey The uppercase key to attempt default insertion for.
   */
  void attemptDefaultInsertion_NoLock(const std::string &upKey) const;

  /**
   * @brief Converts a string to type T using std::istringstream with std::boolalpha.
   *
   * @tparam T The target type to parse.
   * @param s The source string.
   * @param key The key name for error logging.
   * @return Parsed value of type T.
   * @throws std::runtime_error if parsing fails.
   */
  template <typename T>
  T convertStringTo(const std::string &s, const std::string &key) const {
    std::istringstream iss(s);
    iss >> std::boolalpha;
    T value;
    if (!(iss >> value)) {
      Logger::instance().log(LogLevel::ERROR,
        "Context::convertStringTo -> Failed converting \"" + s + "\" for key=" + key);
      throw std::runtime_error("Conversion failed for key=" + key + ", value=\"" + s + "\"");
    }
    return value;
  }

  /**
   * @brief Converts a string to uppercase (basic ASCII).
   */
  static std::string uppercase(const std::string &input) {
    std::string result = input;
    for (size_t i = 0; i < result.size(); ++i) {
      result[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(result[i])));
    }
    return result;
  }

private:
  /**
   * @brief Internal map of uppercase key -> list of string values.
   *
   * Declared mutable so that we can insert defaults even from const getters.
   */
  mutable std::map<std::string, std::vector<std::string>> storage_;

  /**
   * @brief Reference to the blueprint map, cannot be reseated in a copy.
   */
  const std::unordered_map<std::string, BlueprintItem>& allowedKeys_;

  /**
   * @brief Mutex for thread safety on storage_.
   */
  mutable std::mutex mtx_;
};

} // namespace core
} // namespace tadah

#endif // TADAH_CORE_CONTEXT_H

