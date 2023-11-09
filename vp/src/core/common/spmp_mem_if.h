#ifndef RISCV_VP_SPMP_MEM_IF_H
#define RISCV_VP_SPMP_MEM_IF_H

#include <stdint.h>
#include "core/common/protected_access.h"

struct spmp_memory_if {
    virtual ~spmp_memory_if() {}

    virtual bool phya_spmp_check(PrivilegeLevel mode, uint64_t paddr, uint32_t sz, MemoryAccessType type) = 0;
};

#endif //RISCV_VP_SPMP_MEM_IF_H
