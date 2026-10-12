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
#include <iterator>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <tadah/core/blueprint_parser.h>
#include <tadah/core/logger.h>
#include <tadah/core/number_format.h>
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
   * Value i is written to out[i], converted as get<T>() converts (an
   * integer element takes an exact integer only, see convertStringTo()).
   * A key with fewer stored values than the container holds fills those
   * and leaves the other elements as they are; a key with more is refused
   * before any element is written.
   *
   * @tparam Container type with operator[] and size() (e.g.,
   *         std::vector<T>), or an array (std::size()).
   * @param key Key name.
   * @param out The container to fill.
   * @throws std::runtime_error if the key is missing, stores more values
   *         than std::size(out), or a conversion fails.
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
    const size_t capacity = static_cast<size_t>(std::size(out));
    if (vals.size() > capacity) {
      const std::string msg =
          "Context::get: key " + upKey + " stores " +
          std::to_string(vals.size()) + " values, but the container it is " +
          "read into holds " + std::to_string(capacity);
      Logger::instance().log(LogLevel::ERROR, msg);
      throw std::runtime_error(msg);
    }
    using Elem = std::remove_cv_t<std::remove_reference_t<decltype(out[0])>>;
    for (size_t i = 0; i < vals.size(); ++i) {
      if constexpr (is_strict_integral_v<Elem>) {
        out[i] = convertStringTo<Elem>(vals[i], upKey);
      } else {
        std::istringstream iss(vals[i]);
        iss >> out[i];
        if (iss.fail()) {
          Logger::instance().log(
              LogLevel::ERROR, "Context::get(container) -> Failed converting \"" +
                                   vals[i] + "\" for key=" + upKey);
          throw std::runtime_error("Conversion failed for key=" + upKey +
                                   ", value=\"" + vals[i] + "\"");
        }
      }
    }
  }

  /**
   * @brief Appends a value to an existing key, respecting blueprint capacity.
   *
   * If the key is not allowed by blueprint, or if capacity is reached, throws
   * std::runtime_error.
   *
   * Values are stored as lossless text (to_string_lossless()): a floating
   * value, or each element of a range, comes back bit-identical from
   * get<double>(). Strings are split on whitespace and stored verbatim.
   *
   * @tparam T Scalar, string or range type (see to_string_lossless()).
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
    // Lossless text: a double read back with get<double>() is bit-identical.
    std::string valStr = to_string_lossless(val);
    std::istringstream iss(valStr);
    std::string value;

    if (maxVals > 1 && bucket.size() >= maxVals) {
      Logger::instance().log(LogLevel::ERROR,
                             "Context::add -> capacity exceeded for: ", upKey);
      throw std::runtime_error("Exceeded capacity for key: " + upKey);
    } else if (maxVals == 1 && bucket.size() == maxVals) {
      Logger::instance().log(LogLevel::DEBUG,
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
        bucket.push_back(value);
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
   * @brief True iff the key currently STORES at least one value.
   *
   * Unlike contains(), never inserts a blueprint default: use it to tell a
   * value the user (or a loaded file) supplied from an absent one.
   */
  bool has_stored(const std::string &key) const;

  /**
   * @brief The n-th stored token of a key, without default insertion.
   *
   * @return std::nullopt if the key is not stored or has fewer than n+1
   * values.
   */
  std::optional<std::string> stored_value(const std::string &key,
                                          size_t n = 0) const;

  /**
   * @brief Writes the Context as a potential file body.
   *
   * Same layout as operator<<, except that NUMERIC is never written and
   * every non-integer decimal literal of a key whose blueprint value_type
   * contains DOUBLE is re-rendered with @p sig_digits significant digits
   * ("%.{N}g"; 17 or more round-trip every double exactly). Below 17 the
   * literals of a key the blueprint marks exact_in_potential (RCUT2B,
   * RCUTMB, CGRID2B, CGRIDMB, SGRID2B, SGRIDMB: a load checks them exactly)
   * are written as the shortest text that reads back as the same double
   * instead (reformat_double_token_exact()). Integer literals, strings,
   * grid generator names (LIN/GEOM), bools, index patterns and inf/nan are
   * written verbatim.
   *
   * @param os Output stream.
   * @param sig_digits Significant digits, clamped to [1, kMaxSigDigits];
   *        normally resolve_pot_digits(ctx).digits.
   */
  void write_potential(std::ostream &os, int sig_digits) const;

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

  /* getMapRef() was removed: it returned an unsynchronized reference to
   * storage_ (mutated under mtx_ even by const getters via lazy default
   * insertion), so any caller was an unguarded data race. Its only user was
   * operator==, which now locks both contexts and compares storage_ directly.
   * storage_snapshot() below is the safe replacement: a COPY taken under the
   * lock (used e.g. by the PC_ISOLATION delta-pot transport to diff two
   * potential contexts key-by-key). */

  /**
   * @brief Returns a copy of the key -> values storage map, taken under the
   * internal mutex. Safe to call concurrently with any other member.
   */
  std::map<std::string, std::vector<std::string>> storage_snapshot() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return storage_;
  }

  /**
   * @brief Replaces (or erases) ONE key's raw value tokens -- the keyed
   * counterpart of deserialize(): key uppercased, tokens stored verbatim,
   * NO blueprint validation, no capacity checks (deserialize() has none
   * either). Empty `values` erases the key. Used by the PC_ISOLATION
   * delta-pot transport to apply per-key updates without re-parsing the
   * whole serialized context.
   */
  void storage_update(const std::string &key,
                      const std::vector<std::string> &values) {
    std::lock_guard<std::mutex> lock(mtx_);
    const std::string upKey = to_upper(key);
    if (values.empty()) storage_.erase(upKey);
    else                storage_[upKey] = values;
  }

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
    ctx.print_NoLock(os, false, 0);
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
    /* `merged = ctx1` copies under ctx1.mtx_ (copy ctor). ctx2's storage is
     * snapshotted under ctx2.mtx_ and iterated WITHOUT any lock held, so the
     * merged.contains()/remove()/add() calls below (which take merged.mtx_
     * internally) can never deadlock or order against ctx2's lock.
     *
     * The previous implementation constructed lock_guards with std::adopt_lock
     * WITHOUT ever locking the mutexes -- undefined behaviour (unlocking
     * unowned mutexes; flagged by ThreadSanitizer) and no mutual exclusion. */
    Context merged = ctx1;
    std::map<std::string, std::vector<std::string>> ctx2_snapshot;
    {
      std::lock_guard<std::mutex> lock2(ctx2.mtx_);
      ctx2_snapshot = ctx2.storage_;
    }

    for (const auto &pair : ctx2_snapshot) {
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
            core::LogLevel::DEBUG,
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
    // convert val to string (lossless, see add())
    std::string valStr = to_string_lossless(val);
    std::istringstream iss(valStr);
    std::string value;
    if (bucket.size() == maxVals) {
      Logger::instance().log(LogLevel::DEBUG,
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
      bucket.push_back(value);
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
   * @brief Shared body of operator<< and write_potential(); mtx_ must be
   * locked.
   *
   * With @p potential false every token is written verbatim (operator<<).
   * With @p potential true NUMERIC is skipped and DOUBLE-typed decimal
   * literals are re-rendered with @p sig_digits significant digits, those
   * of an exact_in_potential key exactly (see write_potential()).
   */
  void print_NoLock(std::ostream &os, bool potential, int sig_digits) const;

  /** The integer types an integer read covers (convertStringTo()): every
   *  integral type but bool and the character types. */
  template <typename T>
  static constexpr bool is_strict_integral_v =
      std::is_integral_v<T> && !std::is_same_v<T, bool> &&
      !std::is_same_v<T, char> && !std::is_same_v<T, signed char> &&
      !std::is_same_v<T, unsigned char> && !std::is_same_v<T, wchar_t> &&
      !std::is_same_v<T, char16_t> && !std::is_same_v<T, char32_t>;

  /**
   * @brief The integer a token spells, when it spells one exactly.
   *
   * The token is a decimal number, read whole: [+|-] digits [. digits]
   * [(e|E) [+|-] digits], with at least one digit before the exponent. It is
   * taken when its value is an integer in the range of T, whatever its
   * spelling ("2", "2.0", "1e3", ".5e1"). The value is computed from the
   * digits exactly (no floating-point rounding, independent of the locale).
   *
   * @return false for a fraction ("2.7"), text that is not such a number
   *         ("2abc", "0x10", "inf") and a value out of the range of T.
   */
  template <typename T>
  static bool parse_integral_token(const std::string &s, T &out) {
    static_assert(is_strict_integral_v<T>, "an integer type");
    using U = unsigned long long;
    auto digit = [](char ch) { return ch >= '0' && ch <= '9'; };
    size_t i = 0;
    bool neg = false;
    if (i < s.size() && (s[i] == '+' || s[i] == '-')) neg = s[i++] == '-';
    std::string digits;            // the mantissa digits, without the point
    long point = 0;                // digits before the decimal point
    while (i < s.size() && digit(s[i])) digits += s[i++];
    point = static_cast<long>(digits.size());
    if (i < s.size() && s[i] == '.') {
      ++i;
      while (i < s.size() && digit(s[i])) digits += s[i++];
    }
    if (digits.empty()) return false;
    if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
      ++i;
      bool eneg = false;
      if (i < s.size() && (s[i] == '+' || s[i] == '-')) eneg = s[i++] == '-';
      if (i >= s.size() || !digit(s[i])) return false;
      long e = 0;
      // Clamped: past 10^6 every non-zero mantissa is out of range of T
      // (or a fraction), which the loops below see either way.
      while (i < s.size() && digit(s[i])) {
        if (e < 1000000) e = e * 10 + (s[i] - '0');
        ++i;
      }
      point += eneg ? -e : e;
    }
    if (i != s.size()) return false;
    // A digit after the (shifted) decimal point that is not 0: a fraction.
    for (long k = point < 0 ? 0 : point; k < static_cast<long>(digits.size()); ++k)
      if (digits[static_cast<size_t>(k)] != '0') return false;
    // The magnitude: the digits before the point, then zeros up to it.
    const U max_mag =
        neg ? (std::is_signed_v<T>
                   ? static_cast<U>(std::numeric_limits<T>::max()) + 1
                   : 0)
            : static_cast<U>(std::numeric_limits<T>::max());
    U mag = 0;
    for (long k = 0; k < point; ++k) {
      const U d = k < static_cast<long>(digits.size())
                      ? static_cast<U>(digits[static_cast<size_t>(k)] - '0')
                      : 0;
      if (mag > max_mag / 10 || (mag == max_mag / 10 && d > max_mag % 10))
        return false;   // out of range of T
      mag = mag * 10 + d;
    }
    if (!neg) {
      out = static_cast<T>(mag);
    } else if constexpr (std::is_signed_v<T>) {
      out = mag == max_mag ? std::numeric_limits<T>::min()
                           : static_cast<T>(-static_cast<T>(mag));
    } else {
      out = 0;   // "-0": max_mag is 0, so mag is 0
    }
    return true;
  }

  /**
   * @brief Converts a string to type T.
   *
   * An integer T (is_strict_integral_v) takes the whole token when it spells
   * an exact integer in the range of T (parse_integral_token()): "2", "2.0"
   * and "1e3" are read, "2.7", "2abc" and, for an unsigned T, "-1" are
   * refused. Every other T is read with std::istringstream and
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
    T value{};
    bool ok = false;
    std::string why;
    if constexpr (is_strict_integral_v<T>) {
      ok = parse_integral_token(s, value);
      if (!ok)
        why = std::string(": an integer is required") +
              (std::is_signed_v<T> ? "" : " that is not negative") +
              ", from " + std::to_string(std::numeric_limits<T>::min()) +
              " to " + std::to_string(std::numeric_limits<T>::max());
    } else {
      std::istringstream iss(s);
      iss >> std::boolalpha;
      ok = static_cast<bool>(iss >> value);
    }
    if (!ok) {
      Logger::instance().log(
          LogLevel::ERROR, "Context::convertStringTo -> Failed converting \"" +
                               s + "\" for key=" + key + why);
      throw std::runtime_error("Conversion failed for key=" + key +
                               ", value=\"" + s + "\"" + why);
    }
    return value;
  }

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

  // helper method to check all values for a given allowed key
  bool isPositiveValue(const std::string &allowedKey) const {
    auto it = storage_.find(allowedKey);
    if (it == storage_.end()) {
      return true;
    }
    for (const std::string &valStr : it->second) {
      double val = convertStringTo<double>(valStr, allowedKey);
      if (val < 0) {
        return false;
      }
    }
    return true;
  }

  void updateInternalKeys() {
    if (!isPositiveValue("RCUT2B") ||
        !isPositiveValue("RCUTMB")) {
      Logger::instance().log(LogLevel::ERROR,
                             "Context::updateInternalKeys -> RCUT2B or RCUTMB has negative values.");
      throw std::runtime_error("RCUT2B or RCUTMB has negative values.");
    }
    double maxVal2B = findMaxValue("RCUT2B");
    double maxValMB = findMaxValue("RCUTMB");
    storage_["RCUT2BMAX"] = {format_roundtrip(maxVal2B)};
    storage_["RCUTMBMAX"] = {format_roundtrip(maxValMB)};
    storage_["RCUTMAX"] = {format_roundtrip(std::max(maxVal2B, maxValMB))};
  }

  bool isInternalKey(const std::string &key) const {
    return internalKeys_.find(to_upper(key)) != internalKeys_.end();
  }

};

} // namespace core
} // namespace tadah

#endif // TADAH_CORE_CONTEXT_H
