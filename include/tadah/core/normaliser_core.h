#ifndef TADAH_CORE_NORMALISER_CORE_H
#define TADAH_CORE_NORMALISER_CORE_H

#include <tadah/core/context.h>
#include <tadah/core/core_types.h>
#include <tadah/core/logger.h>

namespace tadah {
namespace core {

// We do calculate and store st. dev. and mean
// for the first column of Phi matrix
// for linear kernel, first column are 1s
// so we have to deal with them here
class Normaliser_Core {
    public:
        bool bias;

        v_type std_dev;
        v_type mean;

        Normaliser_Core ();
        Normaliser_Core (const Context &c);

        /** Normalise AED */
        void normalise(aed_type &aed);

        /** Normalise FD */

        void normalise(fd_type &fd);
        void normalise(fd_type &fd, size_t k);
};
} // namespace core
} // namespace tadah
#endif
