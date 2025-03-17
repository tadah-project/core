#ifndef TADAH_CORE_CONTEXT_H
#define TADAH_CORE_CONTEXT_H

#include <cstddef>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <boost/variant.hpp> // For mixed types support via boost::variant
#include <tadah/core/blueprint_parser.h>
#include <tadah/core/core_types.h> // Provides definitions for Matrix and v_type

namespace tadah {
namespace core {

/**
 * @brief Enumeration of supported configuration types.
 */
enum class ConfigType {
  UNDEFINED,
  INT,
  DOUBLE,
  STRING,
  VECTOR, ///< Represents v_type (vector)
  MATRIX, ///< Represents Matrix (2D array)
  MIXED   ///< Mixed types (vector of int, double, string)
};

/**
 * @brief Variant type used for mixed vector types.
 *
 * Supports int, double, string.
 */
typedef std::vector<boost::variant<int, double, std::string>> mixedVecType;

/**
 * @brief Variant type used for configuration values.
 *
 */
typedef boost::variant<bool, int, double, std::string, v_type, Matrix,
                       mixedVecType>
    ConfigVariant;

/**
 * @brief Class for a configuration value.
 *
 * Encapsulates the value (after type conversion) and provides operators for
 * access.
 */
class ConfigValue {
public:
  /**
   * @brief Default constructs a ConfigValue (type UNDEFINED).
   */
  ConfigValue() : type_(ConfigType::UNDEFINED) {}

  /// Constructors for supported types.
  ConfigValue(int i) : type_(ConfigType::INT), value_(i) {}
  ConfigValue(double d) : type_(ConfigType::DOUBLE), value_(d) {}
  ConfigValue(const std::string &s) : type_(ConfigType::STRING), value_(s) {}
  ConfigValue(const v_type &v) : type_(ConfigType::VECTOR), value_(v) {}
  ConfigValue(const Matrix &m) : type_(ConfigType::MATRIX), value_(m) {}
  ConfigValue(const mixedVecType &mix)
      : type_(ConfigType::MIXED), value_(mix) {}

  /**
   * @brief Retrieves the stored value.
   * @return The boost::variant representing the value.
   */
  ConfigVariant value() const { return value_; }

  /**
   * @brief Returns the configuration type.
   */
  ConfigType type() const { return type_; }

  /**
   * @brief Access operator for vector or matrix types.
   *
   * For type VECTOR, returns the i-th element.
   * For type MATRIX, treats as flattened col-major and returns i-th element.
   *
   * @param i Index.
   * @return The element at index i.
   * @throws std::out_of_range if i is out-of-bounds.
   */
  template <typename T> T operator()(std::size_t i) const {
    if (type_ == ConfigType::VECTOR) {
      // For VECTOR type, use v_type operator()(i) provided by
      // tadah/core/maths.h.
      const v_type &vec = boost::get<v_type>(value_);
      if (i >= vec.size())
        throw std::out_of_range("Index out of bounds in vector.");
      return vec[i];
    } else if (type_ == ConfigType::MATRIX) {
      // For MATRIX, flatten col-major.
      const Matrix &mat = boost::get<Matrix>(value_);
      std::size_t total = mat.size();
      if (i >= total)
        throw std::out_of_range("Index out of bounds in matrix (flattened).");
      return mat[i];
    } else if (type_ == ConfigType::MIXED) {
      const mixedVecType &vec = boost::get<mixedVecType>(value_);
      if (i >= vec.size())
        throw std::out_of_range("Index out of bounds in vector.");
      return vec[i];
    } else {
      throw std::runtime_error(
          "operator() not supported for this ConfigValue type.");
    }
  }

  /**
   * @brief Two-dimensional access for Matrix type.
   *
   * @param i Row index.
   * @param j Column index.
   * @return The element at (i,j).
   * @throws std::out_of_range if indices are invalid.
   */
  double operator()(std::size_t i, std::size_t j) const;

  /**
   * @brief Appends a new element to the vector.
   *
   * Supports only VECTOR type; throws an exception if the stored type is not
   * VECTOR. This method provides a more convenient interface for adding
   * components.
   *
   * @param value The value to append.
   * @throws std::runtime_error if type is not VECTOR.
   */
  template <typename T> void add(T value) {
    if (type_ == ConfigType::VECTOR) {
      // To modify the vector, a non-const reference is obtained.
      v_type &vec = const_cast<v_type &>(boost::get<v_type>(value_));
      vec.push_back(value);
    } else if (type_ == ConfigType::MIXED) {
      // To modify the vector, a non-const reference is obtained.
      mixedVecType &vec = const_cast<mixedVecType &>(boost::get<mixedVecType>(value_));
      vec.push_back(value);
    } else {
      throw std::runtime_error(
          "push_back is only supported for VECTOR and MIXED type configurations.");
    }
  }

  /**
   * @brief Resizes the underlying container.
   *
   * Works for VECTOR.
   * @param n New size.
   */
  void resize(std::size_t n);

  /**
   * @brief Resizes the underlying container.
   *
   * Works for MATRIX types.
   * @param m New row size.
   * @param n New column size.
   */
  void resize(std::size_t m, std::size_t n);

  /**
   * @brief Serializes the value to a string representation.
   *
   * This is useful for debugging and logging.
   * @return A string representing the value.
   */
  std::string serialize() const;

private:
  ConfigType type_;
  ConfigVariant value_;
};

/**
 * @brief Structure to hold metadata for configuration entries.
 */
struct MetaData {
  bool isGlobal; ///< True if the key is global, false if task-specific.
};

/**
 * @brief Class representing a merged configuration context.
 *
 * Stores key-value pairs along with optional metadata.
 * Keys are case-insensitive and stored in uppercase.
 * Provides get, set, remove, clear, reset, and size functions.
 * Caches type-converted values for efficient runtime access.
 * Supports serialization and deserialization.
 */
class Context {
public:
  /**
   * @brief Inserts or updates a configuration key.
   * @param key The key name.
   * @param value The configuration value.
   * @param meta Optional metadata (defaults to global).
   */
  void set(const std::string &key, const ConfigValue &value,
           const MetaData &meta = MetaData{true});

  /**
   * @brief Retrieves the configuration value for a key.
   * @param key The key name.
   * @return The configuration value.
   * @throws std::runtime_error if the key is not found.
   */
  ConfigValue get(const std::string &key) const;

  /**
   * @brief Template convenience getter.
   *
   * Retrieves the configuration value for the specified key as type T.
   *
   * @tparam T The expected type.
   * @param key The key name.
   * @return The configuration value converted to type T.
   * @throws std::runtime_error if the stored type does not match T.
   */
  template <typename T> T getV(const std::string &key) const {
    ConfigValue cv = get(key);
    try {
      return boost::get<T>(cv.value());
    } catch (const boost::bad_get &) {
      throw std::runtime_error("Configuration value for key '" + key +
                               "' is not of expected type.");
    }
  }

  /**
   * @brief Template convenience setter.
   *
   * Inserts or updates the configuration key with the given value.
   *
   * @tparam T The type of value.
   * @param key The key name.
   * @param value The value to store.
   * @param meta Optional metadata (default marks key as global).
   */
  template <typename T>
  void setV(const std::string &key, T value, const MetaData &meta) {
    set(key, ConfigValue(value), meta);
  }

  /**
   * @brief Tests if the context contains a key.
   * @param key The key name.
   * @return True if exists, false otherwise.
   */
  bool contains(const std::string &key) const;

  /**
   * @brief Removes a key from the context.
   * @param key The key name.
   */
  void remove(const std::string &key);

  /**
   * @brief Clears the context.
   *
   * If onlyTask is true, only task-specific keys are cleared.
   * @param onlyTask If true, global keys remain.
   */
  void clear(bool onlyTask = false);

  /**
   * @brief Resets global settings to their default values.
   */
  void resetGlobal();

  /**
   * @brief Returns the number of keys stored in the context.
   * @return the total key count.
   */
  std::size_t size() const;

  /**
   * @brief Returns the size (e.g., number of elements) for a given key.
   *
   * For a scalar, returns 1; for VECTOR or MATRIX, returns container size.
   * @param key The key name.
   * @return The size.
   * @throws std::runtime_error if the key does not exist.
   */
  std::size_t size(const std::string &key) const;

  /**
   * @brief Serializes the entire context to a string.
   * @return A plain text serialization.
   */
  std::string serialize() const;

  /**
   * @brief Deserializes the context from a string.
   * @param data The serialized text.
   */
  void deserialize(const std::string &data);

  /**
   * @brief Outputs the context to an output stream.
   * @param os The output stream.
   * @param ctx The Context object.
   * @return The output stream.
   */
  friend std::ostream &operator<<(std::ostream &os, const Context &ctx);

private:
  mutable std::mutex mtx_;
  std::unordered_map<std::string, ConfigValue> settings_;
  std::unordered_map<std::string, MetaData> metadata_;
  std::unordered_map<std::string, BlueprintItem>
      allowedKeys_; ///< Allowed keys loaded from blueprint
                    ///
  /**
   * @brief Loads allowed keys from the blueprint.
   *
   * The blueprint is parsed (using BlueprintParser) and the allowed keys are
   * stored internally.
   */
  void loadBlueprint(
      const std::string &blueprintFile = "config/config_blueprint.toml");
};

} // namespace core
} // namespace tadah

#endif // TADAH_CORE_CONTEXT_H
