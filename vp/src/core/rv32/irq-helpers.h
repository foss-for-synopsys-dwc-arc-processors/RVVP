#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "config.h"

namespace rv32 {

static inline bool is_upper_bound_valid_minor_iid(uint32_t minor_iid) {
        if (minor_iid >= iss_config::IMSIC_MAX_IRQS)
                return false;

        return true;
}

static inline bool is_valid_minor_iid(uint32_t minor_iid) {
        if (minor_iid >= iss_config::IMSIC_MAX_IRQS)
                return false;

        // minor IID = 0 is not supported
        if (minor_iid == 0)
                return false;

        return true;
}

static inline uint32_t minor_iid_correct_top(uint32_t minor_iid) {
        if (minor_iid >= iss_config::IMSIC_MAX_IRQS)
                return iss_config::IMSIC_MAX_IRQS - 1;
        else
                return minor_iid;
}

}
