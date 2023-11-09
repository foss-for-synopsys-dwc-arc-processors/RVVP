#pragma once

#include "smpu_mem_if.h"

using namespace rv32;

template <typename RVX_ISS>
class GenericSMPU {
    RVX_ISS &core;
    tlm_utils::tlm_quantumkeeper &quantum_keeper;
    sc_core::sc_time clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
    sc_core::sc_time smpu_access_delay = clock_cycle * 3;

    enum {
        SMPU_X_FLAG = (1 << 0),
        SMPU_W_FLAG = (1 << 1),
        SMPU_R_FLAG = (1 << 2),
        SMPU_U_FLAG = (1 << 3)
    };

    enum {
        /* Values > 0, mean valid entry is found. The value itself is attributes */
        RET_ATTRIBUTES_ZERO = 0,
        RET_ATTRIBUTES_ERROR = -1
    };

    int inline convert_attr(int attr, PrivilegeLevel mode)
    {
        bool sum_bit;

        if (mode == VirtualSupervisorMode || mode == VirtualUserMode)
            sum_bit = core.csrs.vsstatus.fields.sum;
        else
            sum_bit = core.csrs.mstatus.fields.sum;

        /* Special case #0 */
        if (!(attr & SMPU_R_FLAG) && (attr & SMPU_W_FLAG) && !(attr & SMPU_X_FLAG)) {
            /* RWX=010 */
            if ((attr & SMPU_U_FLAG) == 0) {
                /* Shared S/U-mode RO */
                return (SMPU_R_FLAG << 3) | SMPU_R_FLAG;
            } else {
                /* Shared S/U-mode RW */
                attr = SMPU_R_FLAG | SMPU_W_FLAG;
                return (attr << 3) | attr;
            }
        }

        /* Special case #1 */
        if (!(attr & SMPU_R_FLAG) && (attr & SMPU_W_FLAG) && (attr & SMPU_X_FLAG)) {
            /* RWX=011 */
            if ((attr & SMPU_U_FLAG) == 0) {
                /* Shared S/U-mode execute, S-mode read */
                attr = (SMPU_X_FLAG << 3) | SMPU_X_FLAG;
                return attr | (SMPU_R_FLAG << 3);
            } else {
                /* Shared S/U-mode read, U-mode execute */
                attr = (SMPU_R_FLAG << 3) | SMPU_R_FLAG;
                return attr | SMPU_X_FLAG;
            }
        }

        /* General case */
        if ((attr & SMPU_U_FLAG) == 0)
            return (attr & (SMPU_R_FLAG | SMPU_W_FLAG | SMPU_X_FLAG)) << 3;
        else {
            attr &= SMPU_R_FLAG | SMPU_W_FLAG | SMPU_X_FLAG;
            if (sum_bit)
                attr |= (attr & (SMPU_R_FLAG | SMPU_W_FLAG)) << 3; // Enforce S-mode RW- attributes
            return attr;
        }
    }

    int inline convert_attr_hs(int attr)
    {
        bool sum_bit = core.csrs.hstatus.fields.vsum;
        bool uflag = attr & SMPU_U_FLAG;

        attr &= SMPU_R_FLAG | SMPU_W_FLAG | SMPU_X_FLAG;

        if (sum_bit) {
            /* Define permissions for VS/VU mode */

            /* Special case #0 */
            if (!(attr & SMPU_R_FLAG) && (attr & SMPU_W_FLAG) && !(attr & SMPU_X_FLAG)) {
                /* RWX=010 */
                if (!uflag) {
                    /* Shared S/U-mode RO */
                    return (SMPU_R_FLAG << 3) | SMPU_R_FLAG;
                } else {
                    /* Shared S/U-mode RW */
                    attr = SMPU_R_FLAG | SMPU_W_FLAG;
                    return (attr << 3) | attr;
                }
            }

            /* Special case #1 */
            if (!(attr & SMPU_R_FLAG) && (attr & SMPU_W_FLAG) && (attr & SMPU_X_FLAG)) {
                /* RWX=011 */
                if (!uflag) {
                    /* Shared S/U-mode execute, S-mode read */
                    attr = (SMPU_X_FLAG << 3) | SMPU_X_FLAG;
                    return attr | (SMPU_R_FLAG << 3);
                } else {
                    /* Shared S/U-mode read, U-mode execute */
                    attr = (SMPU_R_FLAG << 3) | SMPU_R_FLAG;
                    return attr | SMPU_X_FLAG;
                }
            }

            /* General case */
            if (uflag)
                return attr;
            else
                return attr << 3;
        } else {
            if (uflag)
                return (attr << 3) | attr; /* The same permissions for VS/VU-mode */
            else
                return attr << 3; /* VS-mode memory operations, VU-mode accesses are denied */
        }
    }

    void raise_unaligned_exception(uint64_t addr, MemoryAccessType type)
    {
        /* Address shall be aligned by the size of data.
         * In case of unaligned multi-bytes access, all memory operation
         * addresses shall match to the single SMPU region. SMPU hardware shall detect if the
         * memory operation addresses partially fit to the region boundaries and raise an
         * Load address misaligned or Store/AMO address misaligned exception (exception
         * cause 4 and 6) in such case. */
        switch (type) {
            case LOAD:
                raise_trap(EXC_LOAD_ADDR_MISALIGNED, addr); /* SMPU load crossing the region boundary(4) */
                  break;
            case STORE:
                raise_trap(EXC_STORE_AMO_ADDR_MISALIGNED, addr); /* SMPU store/AMO crossing the region boundary(6) */
                break;
            default:
                throw std::runtime_error("[SMPU] unexpected misalligned access type " + std::to_string(type));
                break;
        }
    }

    int inline find_matching_entry(uint64_t *addr, uint32_t sz, MemoryAccessType type,
                struct icsr_smpuaddr *smpuaddr, struct icsr_smpuconf *smpuconf,
                uint32_t smpumask, SmpuLevel level, PrivilegeLevel mode)
    {
        uint32_t start_addr, end_addr;
        uint32_t start_op, end_op;
        uint64_t phy_addr;
        int cur_attr, ret_attr = RET_ATTRIBUTES_ZERO;
        int ii, rv;

        start_op = *addr;
        end_op = *addr + sz - 1;

        for (ii = 0; ii < SMPU_NREGIONS; ii++) {
            if ( (smpumask & (1 << ii)) == 0 )
                continue;

            if (smpuaddr[ii].is_translated_region()) {
                rv = smpuaddr[ii].check_translated_is_matched(start_op, end_op);
                if (rv == icsr_smpuaddr::SMPU_ACCES_DENIED)
                    continue; // go to next region

                cur_attr = smpuconf[ii].get_attr();
                if ((cur_attr & (SMPU_R_FLAG | SMPU_W_FLAG | SMPU_X_FLAG)) == 0)
                    continue; // RWX=000

                if (ret_attr) {
                    /* The SMPU hardware shall generate exception when S/U-mode memory
                     * operation virtual address matches more than one translated SMPU region. */
                    return RET_ATTRIBUTES_ERROR;
                }

                /* Unlike protected (untranslated) SMPU regions, the unaligned memory operation is allowed at the
                 * border of two translated regions. If the multi-bytes memory operation overlaps two translated
                 * SMPU regions, then access permissions required to complete the memory operation must be
                 * allowed by both matched translated regions
                 */
                // ToDo: To add unaligned access support
                if (rv == icsr_smpuaddr::SMPU_ACCES_GRANTED_FOR_FIRST_HALF)
                    throw std::runtime_error("[SMPU] unaligned access not supported yet" + std::to_string(type));
                else if (rv == icsr_smpuaddr::SMPU_ACCES_GRANTED_FOR_SECOND_HALF)
                    throw std::runtime_error("[SMPU] unaligned access not supported yet" + std::to_string(type));

                uint32_t n = smpuaddr[ii].get_n();
                uint32_t mask = (1 << n) - 1;
                phy_addr = *addr & mask;
                phy_addr |= smpuconf[ii].get_paddr() & ~mask;
                phy_addr |= (uint64_t)smpuconf[ii].get_pax() << 32ULL;

                /* In case of unaligned access we use logical AND of attributes for
                 * first and second halves
                 */
                cur_attr = smpuconf[ii].get_attr();
                if (level == SMPU_LEVEL_1)
                    ret_attr = convert_attr(cur_attr, mode);
                else
                    ret_attr = convert_attr_hs(cur_attr);

                *addr = phy_addr; /* Changing VA->PA */
            } else {
                start_addr = smpuaddr[ii].get_addr();
                end_addr = start_addr + smpuconf[ii].get_size();

                if (start_op >= start_addr && end_op <= end_addr) {
                    cur_attr = smpuconf[ii].get_attr();

                    if ((cur_attr & (SMPU_R_FLAG | SMPU_W_FLAG | SMPU_X_FLAG)) == 0)
                        continue; // RWX=000

                    if (ret_attr) {
                        /* The SMPU hardware shall generate exception when S/U-mode memory
                         * operation address matches more than one protected SMPU region */
                        return RET_ATTRIBUTES_ERROR;
                    }

                    if (level == SMPU_LEVEL_1)
                        ret_attr = convert_attr(cur_attr, mode);
                    else
                        ret_attr = convert_attr_hs(cur_attr);
                } else if (end_op >= start_addr && end_op <= end_addr)
                    raise_unaligned_exception(end_op, type);
                else if (start_op >= start_addr && start_op <= end_addr)
                    raise_unaligned_exception(start_op, type);
            }
        }

        return ret_attr;
    }

    bool inline is_access_allowed_for_su(int attr, MemoryAccessType type,
        PrivilegeLevel mode, bool is_hlvx_access, SmpuLevel level)
    {
        bool mxr;

        if (mode == SupervisorMode || mode == VirtualSupervisorMode)
            attr >>= 3;

        if (type == FETCH)
            return attr & SMPU_X_FLAG;
        else if (type == LOAD) {
            if (is_hlvx_access) {
                return attr & SMPU_X_FLAG;
            } else {
                // ToDo: Is it right logic?
                if ((level == SMPU_LEVEL_1) && (mode == VirtualUserMode ||
                        mode == VirtualSupervisorMode))
                    mxr = core.csrs.vsstatus.fields.mxr;
                else
                    mxr = core.csrs.mstatus.fields.mxr;

                if (mxr)
                    return attr & (SMPU_R_FLAG | SMPU_X_FLAG);
                else
                    return attr & SMPU_R_FLAG;
            }
        } else if (type == STORE)
            return attr & SMPU_W_FLAG;
        else
            assert(false && "Invalid access type");
    }

    void raise_exception(SmpuLevel level, MemoryAccessType type, uint64_t addr, uint64_t mtval2_htval = 0)
    {
        if (level == SMPU_LEVEL_2) {
            switch (type) {
                case FETCH:
                    raise_trap(EXC_INSTR_GUEST_PAGE_FAULT, addr, mtval2_htval); /* Instruction SMPU/page fault(12) */
                    break;
                case LOAD:
                    raise_trap(EXC_LOAD_GUEST_PAGE_FAULT, addr, mtval2_htval); /* Load SMPU/page fault(13) */
                    break;
                case STORE:
                    raise_trap(EXC_STORE_AMO_GUEST_PAGE_FAULT, addr, mtval2_htval); /* Store/AMO SMPU/page fault(15) */
                    break;
                default:
                    break;
            }
        } else {
            switch (type) {
                case FETCH:
                    raise_trap(EXC_INSTR_PAGE_FAULT, addr, mtval2_htval); /* Instruction SMPU/page fault(12) */
                    break;
                case LOAD:
                    raise_trap(EXC_LOAD_PAGE_FAULT, addr, mtval2_htval); /* Load SMPU/page fault(13) */
                    break;
                case STORE:
                    raise_trap(EXC_STORE_AMO_PAGE_FAULT, addr, mtval2_htval); /* Store/AMO SMPU/page fault(15) */
                    break;
                default:
                    break;
            }
        }

        throw std::runtime_error("[smpu] unknown access type " + std::to_string(type));
    }


public:

    GenericSMPU(RVX_ISS &core): core(core), quantum_keeper(core.quantum_keeper){}

    bool do_phy_address_check(PrivilegeLevel mode, uint64_t *pa_va_ddr, uint32_t sz,
        MemoryAccessType type, SmpuLevel level = SMPU_LEVEL_1, bool is_hlvx_access = false)
    {
        uint64_t virt_addr = *pa_va_ddr;
        unsigned current_guest;
        int attr;

        if (mode == MachineMode) {
            /* M-mode access is always considered to pass SMPU checks */
            return true;
        } else if (level == SMPU_LEVEL_1 && (mode == VirtualSupervisorMode || mode == VirtualUserMode)) {
            /* The satp.MODE = BARE(0) enables the S-mode MPU for all S-mode and U-mode operations */
            if (core.csrs.vsatp.fields.mode) /* All other values in the satp.MODE field disable the SMPU */
                return false; /* Page-based 32-bit virtual addressing only */

            if (!core.csrs.hstatus.is_imsic_connected())
                return true;
            current_guest = core.csrs.hstatus.get_guest_id();

            if (core.csrs.vsmpumask.reg == 0) {
                /* All regions are disabled */
                if (mode == VirtualSupervisorMode)
                    return true; /* VS-mode memory access is unrestricted, allow access */
                else
                    raise_exception(level, type, virt_addr); /* VU-mode memory access is denied for any memory operation address */
            }

            attr = find_matching_entry(pa_va_ddr, sz, type, core.icsrs_vs.bank[current_guest].smpuaddr,
                core.icsrs_vs.bank[current_guest].smpuconf, core.csrs.vsmpumask.reg, level, mode);
        } else if (level == SMPU_LEVEL_2 && (mode == SupervisorMode || mode == UserMode)) {
            /* Level 2 doesn't control S-mode and U-mode operations */
            return true;
        } else if (level == SMPU_LEVEL_2 && (mode == VirtualSupervisorMode || mode == VirtualUserMode)) {
            /* The satp.MODE = BARE(0) enables the S-mode MPU for all S-mode and U-mode operations */
            if (core.csrs.hgatp.fields.mode) /* All other values in the satp.MODE field disable the SMPU */
                return false; /* Page-based 32-bit virtual addressing only */

            if (core.csrs.hmpumask.reg == 0) {
                /* All regions are disabled */
                if (mode == VirtualSupervisorMode)
                    return true; /* VS-mode memory access is unrestricted, allow access */
                else
                    raise_exception(level, type, virt_addr); /* VU-mode memory access is denied for any memory operation address */
            }

            attr = find_matching_entry(pa_va_ddr, sz, type, core.icsrs_s.hmpuaddr, core.icsrs_s.hmpuconf,
                    core.csrs.hmpumask.reg, level, mode);
        } else if (level == SMPU_LEVEL_1) {
            /* The satp.MODE = BARE(0) enables the S-mode MPU for all S-mode and U-mode operations */
            if (core.csrs.satp.fields.mode) /* All other values in the satp.MODE field disable the SMPU */
                return false; /* Page-based 32-bit virtual addressing only */

            if (core.csrs.smpumask.reg == 0) {
                /* All regions are disabled */
                if (mode == SupervisorMode)
                    return true; /* S-mode memory access is unrestricted, allow access */
                else
                    raise_exception(level, type, virt_addr); /* U-mode memory access is denied for any memory operation address */
            }

            attr = find_matching_entry(pa_va_ddr, sz, type, core.icsrs_s.smpuaddr,
                core.icsrs_s.smpuconf, core.csrs.smpumask.reg, level, mode);
        } else
            throw std::runtime_error("[smpu] unexpected level/mode pair");

        if (attr == RET_ATTRIBUTES_ZERO)
            raise_exception(level, type, virt_addr);

        if (attr == RET_ATTRIBUTES_ERROR) {
            /* S/U-mode memory operation virtual address matches more than one
             * translated SMPU region
             */
            if (level == SMPU_LEVEL_2)
                raise_trap(EXC_SMPU_GUEST_MULTIPLE_TRANSLATIONS, virt_addr);
            else
                raise_trap(EXC_SMPU_MULTIPLE_TRANSLATIONS, virt_addr);
        }

        if (is_access_allowed_for_su(attr, type, mode, is_hlvx_access, level)) {
            return true;
        }

        raise_exception(level, type, virt_addr, *pa_va_ddr >> 2);

        return false;
    }
};
