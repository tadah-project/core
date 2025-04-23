/**
 * @file context.h
 * @brief Thread-safe configuration context respects blueprint defaults.
 *
 * The Context class stores uppercase keys, each mapped to a vector of strings.
 * Keys are validated against a blueprint, which also may provide defaultValue
 * tokens. If a requested key doesn't exist, the blueprint's defaultValue is
 * inserted (if available).
 *
 * This class is move-only:
 *  - Copy constructor/assignment are deleted because of std::mutex and a
 * reference member.
 *  - A custom move constructor/assignment ensures objects can be returned by
 * value.
 */

#ifndef TADAH_CORE_CONTEXT_H
#define TADAH_CORE_CONTEXT_H

#include <algorithm>
#include <cmath>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <tadah/core/blueprint_parser.h>
#include <tadah/core/logger.h>
#include <tadah/core/utils.h>

namespace tadah {
namespace core {

/**
 * @class Context
 * @brief Thread-safe key-value container that consults the blueprint for
 * allowed keys and defaults.
 *
 * - **Case-insensitive** keys are stored as uppercase.
 * - If a key is absent on get(), a default from the blueprint is inserted if
 * available.
 * - The blueprint also enforces a maximum capacity (|value_N[0]|).
 * - Due to storing a std::mutex and a reference to the blueprint, it is
 * **move-only**.
 */
class Context {
public:
  /**
   * @brief Constructs an empty Context referencing the static blueprint map.
   */
  Context();

  /**
   * @brief Move constructor.
   * Transfers ownership of storage_ while retaining the same blueprint
   * reference.
   */
  Context(Context &&other) noexcept;

  /**
   * @brief Move assignment.
   * Transfers ownership of storage_ while retaining the same blueprint
   * reference.
   */
  Context &operator=(Context &&other) noexcept;

  Context(const Context &other)
      : allowedKeys_(other.allowedKeys_), internalKeys_(other.internalKeys_) {
    std::lock_guard<std::mutex> lock(other.mtx_);
    storage_ = other.storage_;
  }

  Context &operator=(const Context &other) {
    if (this != &other) {
      std::lock(mtx_, other.mtx_);
      std::lock_guard<std::mutex> lock1(mtx_, std::adopt_lock);
      std::lock_guard<std::mutex> lock2(other.mtx_, std::adopt_lock);
      storage_ = other.storage_;
      // allowedKeys_ and internalKeys_ remain unchanged.
    }
    return *this;
  }

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
  const std::vector<std::string> &operator()(const std::string &key) const;

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
  template <typename T> T get(const std::string &key) const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::string upKey = to_upper(key);

    auto it = storage_.find(upKey);
    if (it == storage_.end() || it->second.empty()) {
      attemptDefaultInsertion_NoLock(upKey);
      it = storage_.find(upKey);
      if (it == storage_.end() || it->second.empty()) {
        keyNotFoundLogSuggestThrow(key, "get");
      }
    }
    return convertStringTo<T>(it->second[0], upKey);
  }

  /**
   * @brief Returns the n-th value for the key, converted to type T.
   *
   * If absent, attempts to insert a blueprint default, then checks range.
   * Throws std::runtime_error if still missing, or if n is out-of-range, or
   * conversion fails.
   *
   * @tparam T Destination type.
   * @param key Key name.
   * @param n Index in the values vector.
   * @return The converted value.
   */
  template <typename T> T get(const std::string &key, size_t n) const {
    std::lock_guard<std::mutex> lock(mtx_);
    std::string upKey = to_upper(key);

    auto it = storage_.find(upKey);
    if (it == storage_.end() || it->second.empty()) {
      attemptDefaultInsertion_NoLock(upKey);
      it = storage_.find(upKey);
      if (it == storage_.end() || it->second.empty()) {
        keyNotFoundLogSuggestThrow(key, "get");
      }
    }
    if (n >= it->second.size()) {
      Logger::instance().log(LogLevel::ERROR,
          "Context::get(n) -> Key: "+upKey+", index out of range: ", n);
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
    std::string upKey = to_upper(key);

    auto it = storage_.find(upKey);
    if (it == storage_.end() || it->second.empty()) {
      attemptDefaultInsertion_NoLock(upKey);
      it = storage_.find(upKey);
      if (it == storage_.end() || it->second.empty()) {
        keyNotFoundLogSuggestThrow(key, "get");
      }
    }
    const std::vector<std::string> &vals = it->second;
    try {
      for (size_t i = 0; i < vals.size(); ++i) {
        std::istringstream iss(vals[i]);
        iss >> out[i];
        if (iss.fail()) {
          throw std::runtime_error("Conversion failed for key: " + upKey);
        }
      }
    } catch (std::exception &e) {
      Logger::instance().log(
          LogLevel::ERROR,
          "Context::get(container) -> Conversion failed: ", e.what());
      throw std::runtime_error("Conversion failed for key: " + upKey);
    }
  }

  /**
   * @brief Appends a value to an existing key, respecting blueprint capacity.
   *
   * If the key is not allowed by blueprint, or if capacity is reached, throws
   * std::runtime_error.
   *
   * @tparam T Type convertible to string with operator<<.
   * @param key Key name.
   * @param val Value to append.
   */
  template <typename T> void add(const std::string &key, const T &val, size_t maxVals = 0) {
    std::string upKey = to_upper(key);
    // Validate blueprint
    const auto bi = findBlueprintKey(upKey, "ALLOWED_KEYS");
    if (bi == nullptr) {
      const auto bi_ = findBlueprintKey(upKey, "INTERNAL_KEYS");
      if (bi_ != nullptr) {
        Logger::instance().log(
            LogLevel::ERROR,
            "Context::add -> Attempt to add internal key with "
            "add(), HINT use addInternal(): ",
            upKey);
        throw std::runtime_error(
            "Setting of internal keys is not allowed by this method: " + upKey);
      }
      keyNotFoundLogSuggestThrow(key, "add");
    }
    if (!maxVals) maxVals = static_cast<size_t>(std::abs(bi->value_N[0]));

    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<std::string> &bucket = storage_[upKey];
    std::string valStr = to_string(val, fmt18);
    std::istringstream iss(valStr);
    std::string value;

    if (maxVals > 1 && bucket.size() >= maxVals) {
      Logger::instance().log(LogLevel::ERROR,
                             "Context::add -> capacity exceeded for: ", upKey);
      throw std::runtime_error("Exceeded capacity for key: " + upKey);
    } else if (maxVals == 1 && bucket.size() == maxVals) {
      Logger::instance().log(LogLevel::WARNING,
                             "Context::add -> Overwriting: ", upKey);
      iss >> std::boolalpha >> value;
      bucket[0] = value;
    } else {
      while (iss >> std::boolalpha >> value) {
        if (bucket.size() == maxVals) {
          Logger::instance().log(
              LogLevel::ERROR,
              "Context::add -> capacity exceeded for: ", upKey);
          throw std::runtime_error("Exceeded capacity for key: " + upKey);
        }
        bucket.push_back(to_string(value, fmt18));
      }
    }
    updateInternalKeys();
    if (upKey == "VERBOSE") {
      Logger::instance().setLogLevel(static_cast<LogLevel>(std::stoi(value)));
    }
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
   * @brief Deserializes from a vector of chars (plaintext). Clears existing
   * data first.
   *
   * Each line is "KEY value1 value2 ...". Keys must be allowed by blueprint.
   * Over-capacity triggers an error.
   *
   * @param buffer The serialized data.
   * @throws std::runtime_error on invalid key or capacity exceed.
   */
  void deserialize(const std::vector<char> &buffer);

  /**
   * @brief Returns true if both configs are the same.
   */
  bool operator==(const Context &) const;

  /**
   * Return a const reference to the storage map.
   */
  const std::map<std::string, std::vector<std::string>> &getMapRef() const;

  /**
   * @brief Streams the Context to an output stream.
   *
   * Each key is looked up in the blueprint to determine how many values to
   * place per line. If value_N[0] > 0, each value is printed on its own line
   * with the key. If value_N[0] < 0, all values for that key appear on a single
   * line after the key. If value_N[0] == 0 or value_N is empty, it is skipped.
   */
  friend std::ostream &operator<<(std::ostream &os, const Context &ctx) {
    // Lock the mutex for thread-safe iteration over storage_.
    std::lock_guard<std::mutex> lock(ctx.mtx_);

    const std::string space = "     ";

    // Iterate over all stored key-value pairs.
    for (auto const &pair : ctx.storage_) {
      const std::string &key = pair.first;
      const std::vector<std::string> &vals = pair.second;

      // Check the blueprint so we can interpret value_N.
      const auto bi = ctx.findBlueprintKey(key, "ALLOWED_KEYS");
      if (bi == nullptr) {
        // Key not in blueprint => skip.
        continue;
      }
      // Take the first element of value_N for printing logic.
      int valN = bi->value_N[0];

      if (valN > 0) {
        // Print each value on its own line preceded by key.
        // Example:
        // KEY     val1
        // KEY     val2
        // ...
        for (auto const &v : vals) {
          os << key << space << v << "\n";
        }
      } else if (valN < 0) {
        // Print all values on a single line, preceded by key.
        // Example:
        // KEY     val1     val2     val3
        os << key;
        for (auto const &v : vals) {
          os << space << v;
        }
        os << "\n";
      } else {
        // valN == 0 => skip printing entirely, or handle differently if
        // desired.
        continue;
      }
    }
    return os;
  }

  /**
   * @brief Merges two contexts, add/override ALLOWED_KEYS keys from ctx2 into ctx1.
   *
   * @param ctx1 The first context (base).
   * @param ctx2 The second context (overrides).
   * @return A new Context object with merged values.
   */
  static Context merge(const Context &ctx1,
                       const Context &ctx2, bool allowOverwrite = true) {
    Context merged = ctx1;
    std::lock_guard<std::mutex> lock1(ctx1.mtx_, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(ctx2.mtx_, std::adopt_lock);

    for (const auto &pair : ctx2.storage_) {
      const std::string &key = pair.first;
      // only copy allowed keys
      if (ctx1.allowedKeys_.find(key) == ctx1.allowedKeys_.end()) {
        continue;
      }      

      if (merged.contains(key)) {
        if (!allowOverwrite) {
          core::Logger::instance().log(
            core::LogLevel::WARNING,
            "Context::merge -> Attempting to add new values to an existing key: ", key);
        } else {
          core::Logger::instance().log(
            core::LogLevel::WARNING,
            "Context::merge -> Overwriting key: ", key);
          merged.remove(key);
        }
      }

      const std::vector<std::string> &vals = pair.second;
      for (const auto &val : vals) {
        merged.add(key, val);
      }
    }
    return merged;
  }

  /**
   * @brief Method for inserting an internal key (not in allowedKeys_).
   *
   * Internal keys come from [INTERNAL_KEYS] section of the blueprint.
   *
   * The overwriting logic is as follows:
   * - If key is full at the time of call, the entire key is cleared and the
   * default is set. Then values are appended.
   * - Otherwise, values are appended until capacity is reached.
   * - If capacity is exceeded, an error is thrown.
   *
   * @warning This method is not intended for general usage.
   */
  template <typename T> void addInternal(const std::string &key, const T &val) {
    std::string upKey = to_upper(key);

    // We do NOT allow normal blueprint keys here, only internal
    const auto bi = findBlueprintKey(upKey, "INTERNAL_KEYS");
    if (bi == nullptr) {
      Logger::instance().log(
          LogLevel::ERROR,
          "Context::addInternal -> Key is not recognized as internal: ", upKey);
      throw std::runtime_error("Key not recognized as internal: " + upKey);
    }
    size_t maxVals = static_cast<size_t>(std::abs(bi->value_N[0]));

    std::lock_guard<std::mutex> lock(mtx_);
    std::vector<std::string> &bucket = storage_[upKey];
    // convert val to string
    std::string valStr = to_string(val, fmt18);
    std::istringstream iss(valStr);
    std::string value;
    if (bucket.size() == maxVals) {
      Logger::instance().log(LogLevel::WARNING,
                             "Context::add -> Overwriting: ", upKey);
      bucket.clear();
      attemptDefaultInsertion_NoLock(upKey);
    }
    while (iss >> std::boolalpha >> value) {
      if (bucket.size() == maxVals) {
        Logger::instance().log(
            LogLevel::ERROR, "Context::add -> capacity exceeded for: ", upKey);
        throw std::runtime_error("Exceeded capacity for key: " + upKey);
      }
      bucket.push_back(to_string(value, fmt18));
    }
    updateInternalKeys();
  }

private:
  /**
   * @brief Attempts to insert the blueprint's defaultValue for upKey if it
   * exists and is not empty.
   *
   * This method modifies storage_, so it must only be called when mtx_ is
   * locked. Marking storage_ as mutable allows this logic in const methods.
   *
   * @param upKey The uppercase key to attempt default insertion for.
   */
  void attemptDefaultInsertion_NoLock(const std::string &upKey) const;

  /**
   * @brief Converts a string to type T using std::istringstream with
   * std::boolalpha.
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
      Logger::instance().log(
          LogLevel::ERROR, "Context::convertStringTo -> Failed converting \"" +
                               s + "\" for key=" + key);
      throw std::runtime_error("Conversion failed for key=" + key +
                               ", value=\"" + s + "\"");
    }
    return value;
  }

private:
  // Formatting for floating point numbers string conversion.
  size_t fmt18 = 18;

  /**
   * @brief Internal map of uppercase key -> list of string values.
   *
   * Declared mutable so that we can insert defaults even from const getters.
   */
  mutable std::map<std::string, std::vector<std::string>> storage_;

  /**
   * @brief A reference for [ALLOWED_KEYS] loaded from blueprint.
   */
  const std::unordered_map<std::string, BlueprintKey> &allowedKeys_;

  /**
   * @brief A reference for [INTERNAL_KEYS] loaded from blueprint.
   */
  const std::unordered_map<std::string, BlueprintKey> &internalKeys_;

  /**
   * @brief Mutex for thread safety on storage_.
   */
  mutable std::mutex mtx_;

  /**
   * @brief Finds the BlueprintKey for a given key.
   *
   * This method is used internally to find the correct BlueprintKey for a key.
   * It first looks in allowedKeys_, then in internalKeys_.
   *
   * @param upKey The uppercase key to search for.
   * @return A pointer to the BlueprintKey if found, nullptr otherwise.
   */
  const BlueprintKey *
  findBlueprintKey(const std::string &upKey,
                    const std::string &xKeys = "ALLOWED_KEYS") const;

  /**
   * @brief Logs an error message, suggests a similar key then throw.
   *
   * @param key The key that was not found.
   * @param method The method that was called.
   */
  void keyNotFoundLogSuggestThrow(const std::string &key,
                                  const std::string &method) const;

  // helper method to find max value for a given allowed key
  double findMaxValue(const std::string &allowedKey) const {
    auto it = storage_.find(allowedKey);
    double maxVal = 0;
    if (it == storage_.end()) {
      return maxVal;
    }
    for (const std::string &valStr : it->second) {
      double val = convertStringTo<double>(valStr, allowedKey);
      maxVal = val > maxVal ? val : maxVal;
    }
    return maxVal;
  }

  void updateInternalKeys() {
    double maxVal2B = findMaxValue("RCUT2B");
    double maxValMB = findMaxValue("RCUTMB");
    storage_["RCUT2BMAX"] = {to_string(maxVal2B, fmt18)};
    storage_["RCUTMBMAX"] = {to_string(maxValMB, fmt18)};
    storage_["RCUTMAX"] = {to_string(std::max(maxVal2B, maxValMB), fmt18)};
  }
};

} // namespace core
} // namespace tadah

#endif // TADAH_CORE_CONTEXT_H
