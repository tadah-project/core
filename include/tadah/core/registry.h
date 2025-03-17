#ifndef TADAH_CORE_REGISTRY_H
#define TADAH_CORE_REGISTRY_H

#include <map>
#include <functional>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>

namespace tadah {
namespace core {

template<typename T, typename... Args>
using Factory = std::function<T*(Args&...)>;

template<typename Base, typename... Args>
struct Registry {

  using Map = std::map<std::string, Factory<Base, Args...>>;
  static Map registry;

  template<typename T>
  struct Register {
    Register(const std::string& name) {
      registry[name] = [](Args&... args) -> T* { return new T(args...); };
    }
  };
};

template<typename Base, typename... Args>
Base* factory(const std::string& type, Args&... args) {
  auto it = Registry<Base, Args...>::registry.find(type);

  if (it != Registry<Base, Args...>::registry.end()) {
    return (it->second)(args...);
  }

  std::ostringstream oss;
  oss << "Type: " << type << " of base type: " << typeid(Base).name()
    << " does not exist.";

  throw std::runtime_error(oss.str());
}
} // namespace core
} // namespace tadah
#endif
