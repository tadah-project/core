/**
 * @file context.cpp
 * @brief Implementation of the Context class.
 */

#include <tadah/core/context.h>
#include <tadah/core/logger.h>

#include <cmath>
#include <sstream>
#include <stdexcept>

namespace tadah {
namespace core {

///////////////////////////////////////////////////////////////////////////////
// Constructors / Move
///////////////////////////////////////////////////////////////////////////////
Context::Context()
    : allowedKeys_(BlueprintParser::getKeysMapRef()),
      internalKeys_(BlueprintParser::getInternalKeysMapRef()) {
  // nothing else needed
}

Context::Context(Context &&other) noexcept
    : storage_(std::move(other.storage_)), allowedKeys_(other.allowedKeys_),
      internalKeys_(other.internalKeys_) {
  // The mutex is default-constructed in 'this'.
  // 'other' is left valid but with an empty storage_.
}

Context &Context::operator=(Context &&other) noexcept {
  if (this != &other) {
    // Lock our own mutex (not locking other's for simplicity, assuming
    // single-thread usage).
    std::lock_guard<std::mutex> lock(mtx_);
    storage_ = std::move(other.storage_);
    // The blueprint reference remains the same as in the constructor.
  }
  return *this;
}

///////////////////////////////////////////////////////////////////////////////
// operator()(...)
///////////////////////////////////////////////////////////////////////////////
const std::vector<std::string> &
Context::operator()(const std::string &key) const {
  std::lock_guard<std::mutex> lock(mtx_);
  std::string upKey = uppercase(key);
  auto it = storage_.find(upKey);

  if (it == storage_.end() || it->second.empty()) {
    attemptDefaultInsertion_NoLock(upKey);
    it = storage_.find(upKey);
    if (it == storage_.end() || it->second.empty()) {
      keyNotFoundLogSuggestThrow(key, "operator");
    }
  }
  return it->second;
}

///////////////////////////////////////////////////////////////////////////////
// remove(...)
///////////////////////////////////////////////////////////////////////////////
size_t Context::remove(const std::string &key) {
  std::lock_guard<std::mutex> lock(mtx_);
  std::string upKey = uppercase(key);
  auto it = storage_.find(upKey);
  if (it != storage_.end()) {
    storage_.erase(it);
    return 1;
  }
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// clear()
///////////////////////////////////////////////////////////////////////////////
void Context::clear() {
  std::lock_guard<std::mutex> lock(mtx_);
  storage_.clear();
}

///////////////////////////////////////////////////////////////////////////////
// size(key)
///////////////////////////////////////////////////////////////////////////////
size_t Context::size(const std::string &key) const {
  std::lock_guard<std::mutex> lock(mtx_);
  std::string upKey = uppercase(key);
  auto it = storage_.find(upKey);
  if (it == storage_.end() || it->second.empty()) {
    attemptDefaultInsertion_NoLock(upKey);
    it = storage_.find(upKey);
    if (it == storage_.end() || it->second.empty()) {
      keyNotFoundLogSuggestThrow(key, "size");
    }
  }
  return it->second.size();
}

///////////////////////////////////////////////////////////////////////////////
// size()
///////////////////////////////////////////////////////////////////////////////
size_t Context::size() const {
  std::lock_guard<std::mutex> lock(mtx_);
  return storage_.size();
}

///////////////////////////////////////////////////////////////////////////////
// contains(key)
///////////////////////////////////////////////////////////////////////////////
bool Context::contains(const std::string &key) const {
  std::lock_guard<std::mutex> lock(mtx_);
  std::string upKey = uppercase(key);
  auto it = storage_.find(upKey);
  if (it == storage_.end() || it->second.empty()) {
    attemptDefaultInsertion_NoLock(upKey);
    it = storage_.find(upKey);
  }
  return (it != storage_.end() && !it->second.empty());
}

///////////////////////////////////////////////////////////////////////////////
// serialize()
///////////////////////////////////////////////////////////////////////////////
std::vector<char> Context::serialize() const {
  std::lock_guard<std::mutex> lock(mtx_);
  std::ostringstream oss;
  for (auto &entry : storage_) {
    // Format: KEY value1 value2 ...
    oss << entry.first;
    for (size_t i = 0; i < entry.second.size(); ++i) {
      oss << " " << entry.second[i];
    }
    oss << "\n";
  }
  std::string text = oss.str();
  return std::vector<char>(text.begin(), text.end());
}

///////////////////////////////////////////////////////////////////////////////
// deserialize(buffer)
///////////////////////////////////////////////////////////////////////////////
void Context::deserialize(const std::vector<char> &buffer) {
  std::lock_guard<std::mutex> lock(mtx_);
  storage_.clear();

  if (buffer.empty()) {
    return; // no data
  }
  std::string data(buffer.begin(), buffer.end());
  std::istringstream iss(data);

  std::string line;
  while (std::getline(iss, line)) {
    if (line.empty()) {
      continue;
    }
    std::istringstream lineStream(line);
    std::vector<std::string> tokens;
    {
      std::string token;
      while (lineStream >> token) {
        tokens.push_back(token);
      }
    }
    if (tokens.empty()) {
      continue;
    }

    // First token is the key
    std::string upKey = uppercase(tokens[0]);

    // Check blueprint
    /*const auto bi = findBlueprintItem(upKey, "ALLOWED_KEYS");*/
    /*if (bi == nullptr) {*/
    /*  Logger::instance().log(*/
    /*      LogLevel::ERROR, "Context::deserialize -> Key not allowed: ", upKey);*/
    /*  throw std::runtime_error("Key not allowed: " + upKey);*/
    /*}*/
    /*if (bi->value_N.empty()) {*/
    /*  Logger::instance().log(*/
    /*      LogLevel::ERROR,*/
    /*      "Context::deserialize -> Blueprint missing value_N for key: ", upKey);*/
    /*  throw std::runtime_error("Blueprint incomplete for key: " + upKey);*/
    /*}*/
    /*size_t maxVals = static_cast<size_t>(std::abs(bi->value_N[0]));*/

    // Insert tokens as values
    std::vector<std::string> &bucket = storage_[upKey];
    for (size_t i = 1; i < tokens.size(); ++i) {
      /*if (bucket.size() >= maxVals) {*/
      /*  Logger::instance().log(*/
      /*      LogLevel::ERROR,*/
      /*      "Context::deserialize -> Exceeded capacity for key: ", upKey);*/
      /*  throw std::runtime_error("Exceeded capacity for key: " + upKey);*/
      /*}*/
      bucket.push_back(tokens[i]);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// attemptDefaultInsertion_NoLock(upKey)
///////////////////////////////////////////////////////////////////////////////
void Context::attemptDefaultInsertion_NoLock(const std::string &upKey) const {
  const auto bi = findBlueprintItem(upKey, "ALLOWED_KEYS");
  if (bi == nullptr) {
    // Not in blueprint -> no default
    return;
  }
  if (bi->defaultValue.empty()) {
    // No default provided
    return;
  }
  // Check capacity
  size_t maxVals = 0;
  if (!bi->value_N.empty()) {
    maxVals = static_cast<size_t>(std::abs(bi->value_N[0]));
  }

  // Insert default only if we are not at capacity
  // "storage_" is mutable, so we can write inside a const method.
  // Using const_cast would be messy, so "mutable" is the better design choice.
  auto mapIter = storage_.find(upKey);
  if (mapIter == storage_.end()) {
    // Create an empty vector
    storage_.insert(std::make_pair(upKey, std::vector<std::string>()));
    mapIter = storage_.find(upKey);
  }
  std::vector<std::string> &values = mapIter->second;

  if (maxVals == 0) {
    // The blueprint might say 0 means no values are allowed,
    // so do nothing in that case.
    return;
  }
  values = bi->defaultValue; // both are vectors
}
bool Context::operator==(const Context &other) const {
  return storage_ == other.getMapRef();
}
const std::map<std::string, std::vector<std::string>> &
Context::getMapRef() const {
  return storage_;
}

//-------------------------------------------------------------
// findBlueprintItem(upKey):
// returns pointer to either allowed item or internal item
//-------------------------------------------------------------
const BlueprintItem *
Context::findBlueprintItem(const std::string &upKey,
                           const std::string &xKeys) const {
  if (xKeys == "ALLOWED_KEYS") {
    auto itA = allowedKeys_.find(upKey);
    if (itA != allowedKeys_.end()) {
      return &(itA->second);
    } else {
      return nullptr;
    }
  }

  if (xKeys == "INTERNAL_KEYS") {
    auto itI = internalKeys_.find(upKey);
    if (itI != internalKeys_.end()) {
      return &(itI->second);
    } else {
      return nullptr;
    }
  }
  // Uknown keyMap
  return nullptr;
}

void Context::keyNotFoundLogSuggestThrow(const std::string &key,
                                         const std::string &method) const {
  Logger::instance().log(LogLevel::ERROR,
                         "Context::" + method + "() -> Key not allowed: ", key);
  std::string suggested = BlueprintParser::findClosestKey(key, allowedKeys_);
  if (!suggested.empty()) {
    Logger::instance().log(LogLevel::ERROR, "  Did you mean: ", suggested);
  }

  throw std::runtime_error("Key not allowed: " + key);
}

} // namespace core
} // namespace tadah
