#pragma once

namespace iss_config {

constexpr unsigned MAX_GUEST = 8;
constexpr unsigned IMSIC_MAX_IRQS = 2048;

/* IVT size for interrupts traps in NV (3) mode */
constexpr unsigned NV_MODE_MAX_VECTOR = 255;
}
