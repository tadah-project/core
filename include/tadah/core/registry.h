#ifndef TADAH_CORE_REGISTRY_H
#define TADAH_CORE_REGISTRY_H

#include <map>
#include <functional>
#include <stdexcept>
#include <string>
#include <sstream>
#include <utility>        // std::forward
#include <typeinfo>

template<typename Base, typename... Args>
using Factory = std::function<Base*(Args...)>;

namespace tadah { 
namespace core { 

template<typename Base, typename... Args>
  struct Registry
  {
    using Map = std::map<std::string, Factory<Base, Args...>>;
    static Map registry;

    template<typename Derived>
      struct Register
      {
        Register(const std::string& name)
        {
          /* 2.  Perfect-forward the arguments – works for l-values,
           *     const l-values and r-values equally well.
           */
          registry[name] = [](Args... args) -> Derived*
          {
            return new Derived(std::forward<Args>(args)...);
          };
        }
      };
  };

/* 3.  The factory itself is now also a perfect-forwarding function.
 *     The exact type of the arguments (const / non-const, l-value /
 *     r-value) is preserved and therefore selects the right registry
 *     instance automatically.
 */
template<typename Base, typename... Args>
  Base* factory(const std::string& type, Args&&... args)
  {
    using Reg   = Registry<Base, Args...>;
    auto  it    = Reg::registry.find(type);

    if (it != Reg::registry.end())
      return (it->second)(std::forward<Args>(args)...);

    std::ostringstream oss;
    oss << "Type \"" << type << "\" of base \"" << typeid(Base).name()
      << "\" is not registered.";
    throw std::runtime_error(oss.str());
  }

}   // namespace core
}   // namespace tadah
#endif
