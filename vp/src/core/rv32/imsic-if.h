#pragma once

#include <stdint.h>

#include "core/common/irq_if.h"

struct imsic_mem_target {
	virtual ~imsic_mem_target() {}

        virtual void route_imsic_write(PrivilegeLevel target_imsic, unsigned guest_index, uint32_t value) = 0;
};
