#include <tadah/core/normaliser_core.h>

namespace tadah {
namespace core {

Normaliser_Core::Normaliser_Core() {}
Normaliser_Core::Normaliser_Core(const Context &c)
    : bias(c.get<bool>("BIAS")) {
  if (c.contains("NORM") && c.get<bool>("NORM")) {
    if (c.contains("NSTDEV")) {
      std_dev.resize(c.size("NSTDEV"));
      mean.resize(c.size("NMEAN"));
      c.get<v_type>("NSTDEV", std_dev);
      c.get<v_type>("NMEAN", mean);
    }
  }
}

/** Normalise AED */
void Normaliser_Core::normalise(aed_type &aed) {
  size_t n = aed.size();
  if (bias) {
    for (size_t i = 1; i < n; ++i) {
      aed[i] = (aed[i] - mean[i]) / std_dev[i];
    }
  } else {
    for (size_t i = 0; i < n; ++i) {
      aed[i] = (aed[i] - mean[i]) / std_dev[i];
    }
  }
}

/** Normalise FD */
void Normaliser_Core::normalise(fd_type &fd) {
  // fd is nx3
  size_t n = fd.rows();
  if (bias) {
    for (size_t k = 0; k < 3; ++k) {
      for (size_t i = 1; i < n; ++i) {
        fd(i, k) = fd(i, k) / std_dev[i];
      }
    }
  } else {
    for (size_t k = 0; k < 3; ++k) {
      for (size_t i = 0; i < n; ++i) {
        fd(i, k) = fd(i, k) / std_dev[i];
      }
    }
  }
}
void Normaliser_Core::normalise(fd_type &fd, size_t k) {
  // fd is nx3
  size_t n = fd.rows();
  if (bias) {
    for (size_t i = 1; i < n; ++i) {
      fd(i, k) = fd(i, k) / std_dev[i];
    }
  } else {
    for (size_t i = 0; i < n; ++i) {
      fd(i, k) = fd(i, k) / std_dev[i];
    }
  }
}

} // namespace core
} // namespace tadah
