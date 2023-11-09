#ifndef RISCV_VP_SMPU_MEM_IF_H
#define RISCV_VP_SMPU_MEM_IF_H

#include <stdint.h>
#include "core/common/protected_access.h"

enum SmpuLevel { SMPU_LEVEL_1 = 1, SMPU_LEVEL_2 };

struct smpu_memory_if {
    virtual ~smpu_memory_if() {}

    virtual bool phya_smpu_check(PrivilegeLevel mode, uint64_t *pa_va_ddr, uint32_t sz,
        MemoryAccessType type, SmpuLevel level = SMPU_LEVEL_1, bool is_hlvx_access = false) = 0;
};

#endif //RISCV_VP_SMPU_MEM_IF_H
