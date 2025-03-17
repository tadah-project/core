#ifndef TADAH_CORE_CONTEXT_H
#define TADAH_CORE_CONTEXT_H

#include <string>
#include <unordered_map>
#include <stdexcept>
#include <sstream>
#include <mutex>
#include <cstddef>

// Math utility types are defined in tadah/core/maths.h.
#include <tadah/core/core_types.h>  // Provides definitions for Matrix and v_type

#include <boost/variant.hpp> // For mixed types support via boost::variant

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
  VECTOR,   ///< Represents v_type (vector)
  MATRIX,   ///< Represents Matrix (2D array)
  MIXED     ///< Mixed types (supports boost::variant)
};

/**
 * @brief Variant type used for configuration values.
 *
 * Uses boost::variant to store one of the supported types.
 */
typedef boost::variant<int, double, std::string, v_type, Matrix, std::vector<boost::variant<int, double, std::string>>> ConfigVariant;

/**
 * @brief Class for a configuration value.
 *
 * Encapsulates the value (after type conversion) and provides operators for access.
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
  double operator()(std::size_t i) const;

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
  bool isGlobal;  ///< True if the key is global, false if task-specific.
};

/**
 * @brief Class representing a merged configuration context.
 *
 * Stores key-value pairs along with optional metadata.
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
  void set(const std::string &key, const ConfigValue &value, const MetaData &meta = MetaData{true});

  /**
   * @brief Retrieves the configuration value for a key.
   * @param key The key name.
   * @return The configuration value.
   * @throws std::runtime_error if the key is not found.
   */
  ConfigValue get(const std::string &key) const;

  /**
   * @brief Tests if a key exists in the context.
   * @param key The key name.
   * @return True if exists, false otherwise.
   */
  bool exists(const std::string &key) const;

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
  friend std::ostream& operator<<(std::ostream &os, const Context &ctx);

private:
  mutable std::mutex mtx_;
  std::unordered_map<std::string, ConfigValue> settings_;
  std::unordered_map<std::string, MetaData> metadata_;
};

} // namespace core
} // namespace tadah

#endif // TADAH_CORE_CONTEXT_H

