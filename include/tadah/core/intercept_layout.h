#ifndef TADAH_CORE_INTERCEPT_LAYOUT_H
#define TADAH_CORE_INTERCEPT_LAYOUT_H

#include <tadah/core/context.h>
#include <tadah/core/periodic_table.h>

#include <cstddef>
#include <string>

namespace tadah {
namespace core {

inline size_t n_intercept_columns(const Context &c) {
  if (!c.get<bool>("BIAS")) return 0;
  if (!c.contains("ATOM")) return 0;
  return c.size("ATOM");
}

inline int species_intercept_idx(int Z, const Context &c) {
  if (!c.contains("ATOM")) return -1;
  const size_t n = c.size("ATOM");
  for (size_t i = 0; i < n; ++i) {
    const std::string sym = c.get<std::string>("ATOM", i);
    if (PeriodicTable::find_by_symbol(sym).Z == Z)
      return static_cast<int>(i);
  }
  return -1;
}

} // namespace core
} // namespace tadah

#endif
