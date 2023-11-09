#pragma once

#include <stdint.h>

struct SimulationTrap {
	uint32_t reason;
	unsigned long mtval;
	unsigned long mtval2_htval;
};

inline void raise_trap(uint32_t exc, unsigned long mtval, unsigned long mtval2_htval = 0) {
	throw SimulationTrap({exc, mtval, mtval2_htval});
}
