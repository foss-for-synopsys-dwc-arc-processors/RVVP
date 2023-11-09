#pragma once

#include "spmp_mem_if.h"

struct spmpcfg {
    union {
        uint32_t reg = 0;
        struct {
            unsigned R0 : 1;              // WARL
            unsigned W0 : 1;              // WARL
            unsigned X0 : 1;              // WARL
            unsigned A0 : 2;              // WARL
            unsigned _wiri0 : 2;          // WIRI
            unsigned S0 : 1;              // WARL
            unsigned UNIMPLEMENTED : 24;  // WARL
        } fields;
    };
};

enum {
    ADDRMATCH_OFF = 0,
    ADDRMATCH_TOR,
    ADDRMATCH_NA4,
    ADDRMATCH_NAPOT
};

template <typename RVX_ISS>
class GenericSPMP {
    RVX_ISS &core;
    tlm_utils::tlm_quantumkeeper &quantum_keeper;
    sc_core::sc_time clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
    sc_core::sc_time spmp_access_delay = clock_cycle * 3;

    static constexpr int SPMP_ENTRIES = 64; /* 0, 16 or 64 */
    static constexpr int SPMP_MODES_SZ = 4; /* MachineMode = 3, SupervisorMode = 1, UserMode = 0 */

    struct spmp_cache_entry_str {
        uint64_t rgn_start_addr, rgn_end_addr;
        bool entry_valid;
        spmp_cache_entry_str(): rgn_start_addr(0), rgn_end_addr(0), entry_valid(false) {}
    };

    spmp_cache_entry_str spmp_cache[SPMP_MODES_SZ][MAX_MEM_ACCESS_TYPES];

    enum {
        SPMP_MATCHING_YES = 1,
        SPMP_MATCHING_NO = 0,
        SPMP_MATCHING_PARTIAL = -1
    };

    inline int check_matching (uint64_t addr, uint64_t end, uint64_t start_addr, uint64_t end_addr)
    {
        if (addr >= start_addr && end < end_addr) {
            return SPMP_MATCHING_YES;
        } else if ((addr >= start_addr && addr < end_addr) || (end >= start_addr && end < end_addr)) {
            /* The lowest-numbered SPMP entry that matches any byte of
             * an access determines whether that access succeeds or fails.
             */
            return SPMP_MATCHING_PARTIAL;
        }
        return SPMP_MATCHING_NO;
    }

    int inline find_matching_entry(uint64_t addr, uint32_t sz, struct spmpcfg *cfg,
                uint64_t *rgn_start_addr, uint64_t *rgn_end_addr)
    {
        int ii, jj;
        uint64_t start_addr, end_addr;
        uint64_t val, mask;
        uint64_t end;
        int rv;

        end = addr + sz - 1;
        addr &= ~0x3ULL;
        end  &= ~0x3ULL;

        for (ii = 0; ii < SPMP_ENTRIES; ii++) {
            cfg->reg = core.csrs.spmpcfg[ii >> 2].reg;
            cfg->reg >>= (ii & 0x3) * 8;

            if (cfg->fields.A0 == ADDRMATCH_OFF)
                continue;

            val = core.csrs.spmpswitch[0].reg;
            val |= (uint64_t)core.csrs.spmpswitch[1].reg << 32ULL;
            if ((val & (1ULL << ii)) == 0)
                continue;

            if (cfg->fields.A0 == ADDRMATCH_TOR) {
                if (ii != 0)
                    start_addr = core.csrs.spmpaddr[ii - 1].reg << 2;
                else
                    start_addr = 0;

                end_addr = core.csrs.spmpaddr[ii].reg << 2;

                if (start_addr >= end_addr) {
                    /* PMP entry ii matches no addresses */
                    continue;
                }

                rv = check_matching (addr, end, start_addr, end_addr);
                if (rv == SPMP_MATCHING_YES)
                    break;
                else if (rv == SPMP_MATCHING_PARTIAL)
                    return -1;
            } else if (cfg->fields.A0 == ADDRMATCH_NAPOT) {
                val = core.csrs.spmpaddr[ii].reg;

                /* Finding the least significant zero bit in the address */
                for (jj = 0; jj < (int)(sizeof(uint32_t) * 8); jj++) {
                    if (((val >> jj) & 0x1) == 0)
                        break;
                }
                mask = 1 << (jj + 1); /* The size of the NAPOT range: 4*(2^(jj+1)) */
                end_addr = mask << 2; /* Now end_addr = NAPOT range size */
                mask -= 1; /* Form the mask of NAPOT range */
                start_addr = (val & ~mask) << 2; /* Calculating the start addres of the range */
                end_addr += start_addr; /* Calculating the end addres of the range as start address + size */

                rv = check_matching (addr, end, start_addr, end_addr);
                if (rv == SPMP_MATCHING_YES)
                    break;
                else if (rv == SPMP_MATCHING_PARTIAL)
                    return -1;
            } else if ( cfg->fields.A0 == ADDRMATCH_NA4) {
                start_addr = core.csrs.spmpaddr[ii].reg << 2;
                end_addr = start_addr + 4; /* NA4 is a 4byte range */

                rv = check_matching (addr, end, start_addr, end_addr);
                if (rv == SPMP_MATCHING_YES)
                    break;
                else if (rv == SPMP_MATCHING_PARTIAL)
                    return -1;
            }
        }

        *rgn_start_addr = start_addr;
        *rgn_end_addr = end_addr;

        return ii;
    }

    bool inline check_rwx_permission(struct spmpcfg cfg, MemoryAccessType type)
    {
        if (type == FETCH) {
            return cfg.fields.X0;
        } else if (type == LOAD) {
            return cfg.fields.R0;
        } else if (type == STORE) {
            return cfg.fields.W0;
        } else {
            assert(false && "Invalid access type");
        }
    }

    bool inline is_access_allowed(struct spmpcfg cfg, MemoryAccessType type)
    {
        bool s_bit = cfg.fields.S0;
        bool sum_bit = core.csrs.mstatus.fields.sum;
        auto mode = core.prv;

        if (s_bit == 0) {
            /* U-mode only rule, is enforced on User modes and denied/enforced
             * on Supervisor mode depending on the value of sstatus.SUM
             */

            if (!cfg.fields.R0 && cfg.fields.W0 && cfg.fields.X0) {
                /* Shared RW for S and U */
                return type != FETCH;
            }

            if (!cfg.fields.R0 && cfg.fields.W0 && !cfg.fields.X0) {
                if (mode == SupervisorMode)
                    return type != FETCH;
                else
                    return type == LOAD;
            }

            if (mode == SupervisorMode) {
                if (sum_bit) {
                    if (type != FETCH)
                        return check_rwx_permission(cfg, type);
                    else
                        return false;
                } else {
                    /* Deny for S */
                    return false;
                }
            }

            /* Enforce for U */
            return check_rwx_permission(cfg, type);
        } else {
            /* S-mode only rule, is enforced on Supervisor mode and
             * denied on User mode
             */
            if (cfg.fields.R0 && cfg.fields.W0 && cfg.fields.X0)
                return type == LOAD;

            if (!cfg.fields.R0 && cfg.fields.W0 && cfg.fields.X0) {
                if (mode == UserMode)
                    return type == FETCH;
                /* For S-mode */
                return type != STORE;
            }

            if (!cfg.fields.R0 && cfg.fields.W0 && !cfg.fields.X0)
                return type == FETCH;

            if (!cfg.fields.R0 && !cfg.fields.W0 && !cfg.fields.X0)
                return false; /* Reserved for S and U */

            if (mode == UserMode)
                return false;

            return check_rwx_permission(cfg, type);
        }

        return false;
    }

    void raise_exception(MemoryAccessType type, uint64_t addr)
    {
        /* Failed accesses generate an exception. Reusing exception codes of page fault */
        switch (type) {
            case FETCH:
                raise_trap(EXC_INSTR_PAGE_FAULT, addr); /* Instruction SPMP/page fault(12) */
                break;
            case LOAD:
                raise_trap(EXC_LOAD_PAGE_FAULT, addr); /* Load SPMP/page fault(13) */
                break;
            case STORE:
                raise_trap(EXC_STORE_AMO_PAGE_FAULT, addr); /* Store/AMO SPMP/page fault(15) */
                break;
            default:
                break;
        }

        throw std::runtime_error("[spmp] unknown access type " + std::to_string(type));
    }

public:

    GenericSPMP(RVX_ISS &core): core(core), quantum_keeper(core.quantum_keeper)
    {
    }

    void clear_spmp_cache(void) {
        memset(spmp_cache, 0, sizeof(spmp_cache));
    }

    bool do_phy_address_check(PrivilegeLevel mode, uint64_t paddr, uint32_t sz, MemoryAccessType type)
    {
        struct spmpcfg cfg;
        uint64_t rgn_start_addr, rgn_end_addr;

        if (core.csrs.satp.fields.mode) // 1 means VM enabled
            return false;               // Page-based 32-bit virtual addressing only

        if (mode == VirtualSupervisorMode || mode == VirtualUserMode) {
            /* SPMP doesn't control V-mode, access is alowed */
            return true;
        }

        /* M-mode accesses are always considered to pass SPMP checks */
        if (mode == MachineMode)
            return true;

        /* Check last cached access the same as new */
        if (spmp_cache[mode][type].entry_valid) {
            uint64_t start_addr = paddr & ~0x3ULL;
            uint64_t end_addr   = (paddr + sz - 1) & ~0x3ULL;
            if (start_addr >= spmp_cache[mode][type].rgn_start_addr &&
                        end_addr < spmp_cache[mode][type].rgn_end_addr) {
                /* Cache hit */
                return true;
            }
        }

        /* Optional timing */
        //quantum_keeper.inc(spmp_access_delay);

        int nentry = find_matching_entry(paddr, sz, &cfg, &rgn_start_addr, &rgn_end_addr);

        if (nentry < 0) {
            /* The matching SPMP entry must match all bytes of an access,
                or the access fails, irrespective of the S, R, W, and X bits */
            raise_exception(type, paddr);
        }

        /* If the privilege mode of the access is S and no SPMP nentry matches, the access is allowed */
        if (mode == SupervisorMode && nentry >= SPMP_ENTRIES)
            return true;

        /* If the privilege mode of the access is U and no SPMP nentry matches,
         * but at least one SPMP nentry is implemented, the access is denied
         */
        if (mode == UserMode && nentry >= SPMP_ENTRIES)
            raise_exception(type, paddr);

        if (is_access_allowed(cfg, type)) {
            /* Caching the last successful access */
            spmp_cache[mode][type].rgn_start_addr = rgn_start_addr;
            spmp_cache[mode][type].rgn_end_addr = rgn_end_addr;
            spmp_cache[mode][type].entry_valid = true;
            return true;
        }

        raise_exception(type, paddr);
        return false;
    }
};
