#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <tadah/core/context.h>
#include <vector>

namespace tadah {
namespace core {


double ConfigValue::operator()(std::size_t i, std::size_t j) const {
  if (type_ != ConfigType::MATRIX)
    throw std::runtime_error(
        "Two-dimensional access is only supported for matrix type.");
  const Matrix &mat = boost::get<Matrix>(value_);
  if (i >= mat.rows() || j >= mat.cols())
    throw std::out_of_range("Matrix indices out of bounds.");
  return mat(i, j);
}

void ConfigValue::resize(std::size_t n) {
  if (type_ == ConfigType::VECTOR) {
    v_type &vec = const_cast<v_type &>(boost::get<v_type>(value_));
    vec.resize(n);
  } else {
    throw std::runtime_error("Resize not supported for this ConfigValue type.");
  }
}

void ConfigValue::resize(std::size_t m, std::size_t n) {
  if (type_ == ConfigType::MATRIX) {
    Matrix &mat = const_cast<Matrix &>(boost::get<Matrix>(value_));
    mat.resize(m, n);
  } else {
    throw std::runtime_error("Resize not supported for this ConfigValue type.");
  }
}

std::string ConfigValue::serialize() const {
  std::ostringstream oss;
  oss << "Type:" << static_cast<int>(type_) << ";";
  switch (type_) {
  case ConfigType::INT:
    oss << boost::get<int>(value_);
    break;
  case ConfigType::DOUBLE:
    oss << boost::get<double>(value_);
    break;
  case ConfigType::STRING:
    oss << boost::get<std::string>(value_);
    break;
  case ConfigType::VECTOR: {
    const v_type &vec = boost::get<v_type>(value_);
    for (auto d : vec) {
      oss << d << "  ";
    }
    break;
  }
  case ConfigType::MATRIX: {
    const Matrix &mat = boost::get<Matrix>(value_);
    oss << mat;
  }
  case ConfigType::MIXED: {
    // Not fully implemented—placeholder.
    oss << "MixedValue";
    break;
  }
  default:
    break;
  }
  return oss.str();
}

void Context::set(const std::string &key, const ConfigValue &value,
                  const MetaData &meta) {
  // Convert key to uppercase.
  std::string ukey = key;
  std::transform(ukey.begin(), ukey.end(), ukey.begin(), ::toupper);

  // Check if the key exists in the allowed keys member.
  if (allowedKeys_.find(ukey) == allowedKeys_.end()) {
    throw std::runtime_error("Context::set: Key '" + ukey +
                             "' is not allowed by the blueprint.");
  }

  std::lock_guard<std::mutex> lock(mtx_);
  settings_[ukey] = value;
  metadata_[ukey] = meta;
}

ConfigValue Context::get(const std::string &key) const {
  std::string ukey = key;
  std::transform(ukey.begin(), ukey.end(), ukey.begin(), ::toupper);
  std::lock_guard<std::mutex> lock(mtx_);
  auto it = settings_.find(ukey);
  if (it == settings_.end()) {
    // Check if the key is allowed.
    if (allowedKeys_.find(ukey) == allowedKeys_.end()) {
      throw std::runtime_error(
          "Context::get: Key '" + ukey +
          "' is not allowed by the blueprint. Possibly a typo?");
    } else {
      throw std::runtime_error(
          "Context::get: Key '" + ukey +
          "' is allowed but not defined in the configuration.");
    }
  }
  return it->second;
}

bool Context::contains(const std::string &key) const {
  std::string ukey = key;
  std::transform(ukey.begin(), ukey.end(), ukey.begin(), ::toupper);
  std::lock_guard<std::mutex> lock(mtx_);
  return settings_.find(ukey) != settings_.end();
}

void Context::remove(const std::string &key) {
  std::string ukey = key;
  std::transform(ukey.begin(), ukey.end(), ukey.begin(), ::toupper);
  std::lock_guard<std::mutex> lock(mtx_);
  settings_.erase(ukey);
  metadata_.erase(ukey);
}

void Context::clear(bool onlyTask) {
  std::lock_guard<std::mutex> lock(mtx_);
  if (!onlyTask) {
    settings_.clear();
    metadata_.clear();
  } else {
    for (auto it = settings_.begin(); it != settings_.end();) {
      if (!metadata_[it->first].isGlobal)
        it = settings_.erase(it);
      else
        ++it;
    }
  }
}

void Context::resetGlobal() {
  // Resets task-specific keys.
  clear(true);
}

std::size_t Context::size() const {
  std::lock_guard<std::mutex> lock(mtx_);
  return settings_.size();
}

std::size_t Context::size(const std::string &key) const {
  std::string ukey = key;
  std::transform(ukey.begin(), ukey.end(), ukey.begin(), ::toupper);
  std::lock_guard<std::mutex> lock(mtx_);
  if (!contains(ukey))
    throw std::runtime_error("Key not found: " + ukey);
  // For scalar types return 1, for vector or matrix, the container size is
  // returned.
  ConfigType t = get(ukey).type();
  if (t == ConfigType::VECTOR) {
    return boost::get<v_type>(get(ukey).value()).size();
  } else if (t == ConfigType::MATRIX) {
    // Return total number of elements.
    const Matrix &mat = boost::get<Matrix>(get(ukey).value());
    return mat.size();
  }
  return 1;
}

std::string Context::serialize() const {
  std::lock_guard<std::mutex> lock(mtx_);
  std::ostringstream oss;
  for (const auto &entry : settings_) {
    oss << entry.first << "=" << entry.second.serialize() << "\n";
  }
  return oss.str();
}

void Context::deserialize(const std::string &data) {
  std::istringstream iss(data);
  std::string line;
  std::lock_guard<std::mutex> lock(mtx_);
  settings_.clear();
  metadata_.clear();
  while (std::getline(iss, line)) {
    auto pos = line.find('=');
    if (pos == std::string::npos)
      continue;
    std::string key = line.substr(0, pos);
    std::string valStr = line.substr(pos + 1);
    // For demonstration, store as a string-type ConfigValue.
    settings_[key] = ConfigValue(valStr);
    metadata_[key] = MetaData{true};
  }
}

std::ostream &operator<<(std::ostream &os, const Context &ctx) {
  os << ctx.serialize();
  return os;
}

void Context::loadBlueprint(const std::string &blueprintFile) {
  BlueprintParser bp(blueprintFile);
  allowedKeys_ = bp.parse();
}

} // namespace core
} // namespace tadah
