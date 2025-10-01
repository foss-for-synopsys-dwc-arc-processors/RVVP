#pragma once

#include <bitset>

#include <tlm_utils/simple_target_socket.h>
#include <systemc>

#include "core/common/irq_if.h"
#include "util/memory_map.h"
#include "util/tlm_map.h"

#include "trap-codes.h"


#define HART_0_INDEX                        (0)

#define APLIC_DOMAIN_OFFSET                 (0x10000)

#define APLIC_M_DOMAIN                      (0)
#define APLIC_S_DOMAIN                      (1)

#define APLIC_DOMAINCFG                     (0x0000)
#define APLIC_SOURCECFG                     (0x0004)
#define APLIC_MMSIADDRCFG                   (0x1BC0)
#define APLIC_MMSIADDRCFGH                  (0x1BC4)
#define APLIC_SMSIADDRCFG                   (0x1BC8)
#define APLIC_SMSIADDRCFGH                  (0x1BCC)
#define APLIC_SETIP                         (0x1C00)
#define APLIC_SETIPNUM                      (0x1CDC)
#define APLIC_IN_CLRIP                      (0x1D00)
#define APLIC_CLRIPNUM                      (0x1DDC)
#define APLIC_SETIE                         (0x1E00)
#define APLIC_SETIENUM                      (0x1EDC)
#define APLIC_CLRIE                         (0x1F00)
#define APLIC_CLRIENUM                      (0x1FDC)
#define APLIC_SETIPNUMLE                    (0x2000)
#define APLIC_SETIPNUMBE                    (0x2004)
#define APLIC_GENMSI                        (0x3000)
#define APLIC_TARGET                        (0x3004)

#define APLIC_IDC_BASE                      (0x4000)
#define APLIC_IDC_IDELIVERY                 (APLIC_IDC_BASE + 0x0000)
#define APLIC_IDC_IFORCE                    (APLIC_IDC_BASE + 0x0004)
#define APLIC_IDC_ITHRESHOLD                (APLIC_IDC_BASE + 0x0008)
#define APLIC_IDC_TOPI                      (APLIC_IDC_BASE + 0x0018)
#define APLIC_IDC_CLAIMI                    (APLIC_IDC_BASE + 0x001C)

#define BIT(n)                              (1ULL << (n))
#define BITS_PER_UNSIGNED_LONG              (sizeof(unsigned long) * 8)
#define GENMASK(h, l)                       (((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_UNSIGNED_LONG - 1 - (h))))

/* Bit definitions */
#define APLIC_DOMAINCFG_BE_SHIFT            0
#define APLIC_DOMAINCFG_BE_BIT              BIT(APLIC_DOMAINCFG_BE_SHIFT)
#define APLIC_DOMAINCFG_DM_SHIFT            2
#define APLIC_DOMAINCFG_DM_BIT              BIT(APLIC_DOMAINCFG_DM_SHIFT)
#define APLIC_DOMAINCFG_IE_SHIFT            8
#define APLIC_DOMAINCFG_IE_BIT              BIT(APLIC_DOMAINCFG_IE_SHIFT)
#define APLIC_DOMAINCFG_RO80_SHIFT          24
#define APLIC_DOMAINCFG_RO80_MASK           0xFF

#define APLIC_SOURCECFG_CHILDIND_SHIFT      0
#define APLIC_SOURCECFG_CHILDIND_MASK       0x3FF
#define APLIC_SOURCECFG_SM_SHIFT            0
#define APLIC_SOURCECFG_SM_MASK             0x7
#define APLIC_SOURCECFG_D_SHIFT             10
#define APLIC_SOURCECFG_D_BIT               BIT(APLIC_SOURCECFG_D_SHIFT)
// UIA-specific
#define APLIC_SOURCECFG_IE_SHIFT            31
#define APLIC_SOURCECFG_IP_SHIFT            30
#define APLIC_SOURCECFG_IPRIO_SHIFT         16

#define APLIC_ITHRESHOLD_ITRSH_SHIFT        0
#define APLIC_ITHRESHOLD_ITRSH_MASK         0xFF
#define APLIC_ITHRESHOLD_PTRSH_SHIFT        8
#define APLIC_ITHRESHOLD_PTRSH_MASK         0xFF


#define APLIC_TOPI_IDENTITY_SHIFT           16
#define APLIC_TOPI_PRIO_SHIFT               0

#define APLIC_IDELIVERY_ENABLE_BIT          BIT(0)
#define APLIC_IDELIVERY_PRIMARY_BIT         BIT(31)
#define APLIC_IDELIVERY_LLEN_SHIFT          16
#define APLIC_IDELIVERY_LLEN_MASK           0x7
#define APLIC_IDELIVERY_RW_MASK             (APLIC_IDELIVERY_ENABLE_BIT | APLIC_IDELIVERY_PRIMARY_BIT | (APLIC_IDELIVERY_LLEN_MASK << APLIC_IDELIVERY_LLEN_SHIFT))

#define APLIC_IDC_ONLY_COMPLETE             (APLIC_IDC_BASE + 0x0010)
#define APLIC_IDC_ONLY_CLAIM                (APLIC_IDC_BASE + 0x0014)
#define APLIC_IDC_DEBUG_HW_THRESHOLD        (APLIC_IDC_BASE + 0x0020)

struct APLIC_UIA_DOMAIN {
    RegisterRange *regs_domaincfg;
    IntegerView<uint32_t> *domaincfg;

    RegisterRange *regs_sourcecfg;
    ArrayView<uint32_t> *sourcecfg;

    RegisterRange *regs_reserved_0;
    RegisterRange *regs_setipnum;
    RegisterRange *regs_reserved_1;
    RegisterRange *regs_clripnum;
    RegisterRange *regs_reserved_2;
    RegisterRange *regs_setienum;
    RegisterRange *regs_reserved_3;
    RegisterRange *regs_clrienum;
    RegisterRange *regs_reserved_4;

    // IDC structure
    RegisterRange *regs_idelivery;
    RegisterRange *regs_iforce;
    RegisterRange *regs_ithreshold;
    RegisterRange *regs_topi;
    RegisterRange *regs_claimi;

    // Temporary extra registers
    RegisterRange *regs_only_complete;
    RegisterRange *regs_only_claim;

    // Debug temporary registers
    RegisterRange *regs_debug_hw_threshold;

    static constexpr unsigned IDC_SIZE = 5 * sizeof(uint32_t);

    uint32_t domain_idx;
    bool trace_mode = false;

    APLIC_UIA_DOMAIN(uint32_t domain_idx) : domain_idx(domain_idx) {
        uint32_t domain_base = APLIC_DOMAIN_OFFSET * domain_idx;


        regs_domaincfg = new RegisterRange{domain_base + APLIC_DOMAINCFG, sizeof(uint32_t)};
        regs_domaincfg->alignment = 4;
        domaincfg = new IntegerView<uint32_t>(*regs_domaincfg);

        regs_sourcecfg = new RegisterRange{domain_base + APLIC_SOURCECFG, sizeof(uint32_t) * 1023};
        regs_sourcecfg->alignment = 4;
        sourcecfg = new ArrayView<uint32_t>(*regs_sourcecfg);

        regs_reserved_0 = new RegisterRange{domain_base + APLIC_MMSIADDRCFG, sizeof(uint32_t) * (4 + 32)};
        regs_reserved_0->alignment = 4;
        regs_reserved_0->pre_read_callback = std::bind(&APLIC_UIA_DOMAIN::pre_read_ro0, this, std::placeholders::_1);

        regs_setipnum = new RegisterRange{domain_base + APLIC_SETIPNUM, sizeof(uint32_t)};
        regs_setipnum->alignment = 4;
        regs_setipnum->pre_read_callback = std::bind(&APLIC_UIA_DOMAIN::pre_read_ro0, this, std::placeholders::_1);

        regs_reserved_1 = new RegisterRange{domain_base + APLIC_IN_CLRIP, sizeof(uint32_t) * 32};
        regs_reserved_1->alignment = 4;
        regs_reserved_1->pre_read_callback = std::bind(&APLIC_UIA_DOMAIN::pre_read_ro0, this, std::placeholders::_1);

        regs_clripnum = new RegisterRange{domain_base + APLIC_CLRIPNUM, sizeof(uint32_t)};
        regs_clripnum->alignment = 4;
        regs_clripnum->pre_read_callback = std::bind(&APLIC_UIA_DOMAIN::pre_read_ro0, this, std::placeholders::_1);

        regs_reserved_2 = new RegisterRange{domain_base + APLIC_SETIE, sizeof(uint32_t) * 32};
        regs_reserved_2->alignment = 4;
        regs_reserved_2->pre_read_callback = std::bind(&APLIC_UIA_DOMAIN::pre_read_ro0, this, std::placeholders::_1);

        regs_setienum = new RegisterRange{domain_base + APLIC_SETIENUM, sizeof(uint32_t)};
        regs_setienum->alignment = 4;
        regs_setienum->pre_read_callback = std::bind(&APLIC_UIA_DOMAIN::pre_read_ro0, this, std::placeholders::_1);

        regs_reserved_3 = new RegisterRange{domain_base + APLIC_CLRIE, sizeof(uint32_t) * 32};
        regs_reserved_3->alignment = 4;
        regs_reserved_3->pre_read_callback = std::bind(&APLIC_UIA_DOMAIN::pre_read_ro0, this, std::placeholders::_1);

        regs_clrienum = new RegisterRange{domain_base + APLIC_CLRIENUM, sizeof(uint32_t)};
        regs_clrienum->alignment = 4;
        regs_clrienum->pre_read_callback = std::bind(&APLIC_UIA_DOMAIN::pre_read_ro0, this, std::placeholders::_1);

        regs_reserved_4 = new RegisterRange{domain_base + APLIC_SETIPNUMLE, sizeof(uint32_t) * (1024 * 2)};
        regs_reserved_4->alignment = 4;
        regs_reserved_4->pre_read_callback = std::bind(&APLIC_UIA_DOMAIN::pre_read_ro0, this, std::placeholders::_1);


        // IDC structure
        regs_idelivery = new RegisterRange{domain_base + APLIC_IDC_IDELIVERY, sizeof(uint32_t)};
        regs_idelivery->alignment = 4;

        regs_iforce = new RegisterRange{domain_base + APLIC_IDC_IFORCE, sizeof(uint32_t)};
        regs_iforce->alignment = 4;
        // iforce is not implemented, read as 0
        regs_iforce->pre_read_callback = std::bind(&APLIC_UIA_DOMAIN::pre_read_ro0, this, std::placeholders::_1);

        regs_ithreshold = new RegisterRange{domain_base + APLIC_IDC_ITHRESHOLD, sizeof(uint32_t)};
        regs_ithreshold->alignment = 4;

        regs_topi = new RegisterRange{domain_base + APLIC_IDC_TOPI, sizeof(uint32_t)};
        regs_topi->alignment = 4;
        regs_topi->pre_write_callback = std::bind(&APLIC_UIA_DOMAIN::pre_write_ro, this, std::placeholders::_1);

        regs_claimi = new RegisterRange{domain_base + APLIC_IDC_CLAIMI, sizeof(uint32_t)};
        regs_claimi->alignment = 4;

        // Temporary extra registers
        regs_only_complete = new RegisterRange{domain_base + APLIC_IDC_ONLY_COMPLETE, sizeof(uint32_t)};
        regs_only_complete->alignment = 4;

        regs_only_claim = new RegisterRange{domain_base + APLIC_IDC_ONLY_CLAIM, sizeof(uint32_t)};
        regs_only_claim->alignment = 4;

        regs_debug_hw_threshold = new RegisterRange{domain_base + APLIC_IDC_DEBUG_HW_THRESHOLD, sizeof(uint32_t)};
        regs_debug_hw_threshold->alignment = 4;
    }

    void map_register_ranges(std::vector<RegisterRange *> &register_ranges) {
        register_ranges.push_back(regs_domaincfg);
        register_ranges.push_back(regs_sourcecfg);
        register_ranges.push_back(regs_reserved_0);
        register_ranges.push_back(regs_setipnum);
        register_ranges.push_back(regs_reserved_1);
        register_ranges.push_back(regs_clripnum);
        register_ranges.push_back(regs_reserved_2);
        register_ranges.push_back(regs_setienum);
        register_ranges.push_back(regs_reserved_3);
        register_ranges.push_back(regs_clrienum);
        register_ranges.push_back(regs_reserved_4);

        register_ranges.push_back(regs_idelivery);
        register_ranges.push_back(regs_iforce);
        register_ranges.push_back(regs_ithreshold);
        register_ranges.push_back(regs_topi);
        register_ranges.push_back(regs_claimi);

        register_ranges.push_back(regs_only_complete);
        register_ranges.push_back(regs_only_claim);
        register_ranges.push_back(regs_debug_hw_threshold);
    }

    bool pre_read_ro0(RegisterRange::ReadInfo t) {
        *reinterpret_cast<uint32_t *>(t.var_addr) = 0;

        if (trace_mode)
            std::cout << "[vp::aplic-uia] R access to domain " << domain_idx <<
                " r/o 0 reg at 0x" << std::hex << t.trans.get_address() << std::endl;

        return true;
    }

    bool pre_write_ro(RegisterRange::WriteInfo t) {
        if (trace_mode)
            std::cout << "[vp::aplic-uia] W access to domain " << domain_idx <<
                " r/o reg at 0x" << std::hex << t.trans.get_address() << std::endl;

        return false;
    }
};

struct source_mode {
    enum sm {
        INACTIVE = 0, /* Inactive in this domain (and not delegated) */
        DETACHED = 1, /* Active, detached from the source wire */
        RESERVED1 = 2,
        RESERVED2 = 3,
        EDGE1 = 4, /* Active, edge-sensitive; interrupt asserted on rising edge */
        EDGE0 = 5, /* Active, edge-sensitive; interrupt asserted on falling edge */
        LEVEL1 = 6, /* Active, level-sensitive; interrupt asserted when high */
        LEVEL0 = 7 /* Active, level-sensitive; interrupt asserted when low */
    };

    enum sm mode = INACTIVE;

    void from_sourcecfg(uint32_t sourcecfg) {
        mode = static_cast<enum sm>((sourcecfg >> APLIC_SOURCECFG_SM_SHIFT) & APLIC_SOURCECFG_SM_MASK);
    }

    bool is_inactive() const {
        // RESERVED are treated as INACTIVE
        return mode == INACTIVE || mode == RESERVED1 || mode == RESERVED2;
    }

    bool is_level() const {
        return mode == LEVEL0 || mode == LEVEL1;
    }

    bool is_edge() const {
        return mode == EDGE0 || mode == EDGE1;
    }

    operator std::string() const {
        switch (mode) {
            case INACTIVE: return "INACTIVE";
            case DETACHED: return "DETACHED";
            case RESERVED1: return "RESERVED1";
            case RESERVED2: return "RESERVED2";
            case EDGE1: return "EDGE1";
            case EDGE0: return "EDGE0";
            case LEVEL1: return "LEVEL1";
            case LEVEL0: return "LEVEL0";
            default: return "???";
        }
    }

    friend std::ostream &operator<<(std::ostream &out, const source_mode &sm) {
        out << static_cast<std::string>(sm);
        return out;
    }

};

static constexpr uint8_t PRIO_LEN = 7;

struct composite_prio {
    /* Default - the lowest non-existing priority */
    // TODO: use 0 to mark non-existing priority
    composite_prio() : prio(UINT16_MAX) {}
    composite_prio(uint8_t prio) : prio(prio) {}

    void from_sourcecfg(uint32_t sourcecfg) {
        uint16_t value = (sourcecfg >> APLIC_SOURCECFG_IPRIO_SHIFT) & UINT8_MAX;

        // write 0 to IPRIO sets the priority to 1
        if (value == 0)
            value = 1;

        prio = value;
    }

    uint32_t to_sourcecfg_prio(void) {
        if (is_valid())
            return prio;

        return 1;
    }

    bool operator<(const composite_prio& other) const {
		return this->prio < other.prio;
	}

	bool operator==(const composite_prio& other) const {
		return this->prio == other.prio;
	}

    bool is_valid() const {
        return prio != 0 && prio <= UINT8_MAX;
    }

    bool is_masked_by_threshold(uint8_t threshold) const {
        return prio >= threshold && threshold != 0;
    }

    uint8_t to_prio(void) {
        assert(is_valid());

        return static_cast<uint8_t>(prio);
    }

    operator std::string() const {
        if (!is_valid())
            return "INVALID";

        return std::to_string(prio);
    }

    friend std::ostream &operator<<(std::ostream &out, const composite_prio &cp) {
        out << static_cast<std::string>(cp);
        return out;
    }

private:
    uint16_t prio;
};

struct uia_interrupt {
    uint32_t iid;
    composite_prio prio;

    uia_interrupt() : iid(0), prio() {}
    uia_interrupt(uint32_t iid, composite_prio prio) : iid(iid), prio(prio) {}

    uint32_t to_topi(void) {
        if (iid == 0)
            return 0;

        return (iid << APLIC_TOPI_IDENTITY_SHIFT) | (prio.to_prio() << APLIC_TOPI_PRIO_SHIFT);
    }

    friend std::ostream &operator<<(std::ostream &out, const uia_interrupt &irq) {
        out << "iid=" << std::dec << irq.iid << ", prio=" << irq.prio;
        return out;
    }
};

template <unsigned NumberInterrupts>
struct threshold_mngr {
    bool trace_mode = false;

    void threshold_from_ithreshold(uint32_t ithreshold) {
        sw_threshold = static_cast<uint8_t>(ithreshold & UINT8_MAX);
    }

    void llen_from_idelivery(uint32_t idelivery) {
        uint8_t llen = static_cast<uint8_t>((idelivery >> APLIC_IDELIVERY_LLEN_SHIFT) & APLIC_IDELIVERY_LLEN_MASK);

        // idelivery.LLEN shall be implemented to hold values in a range (0..PRIO_LEN).
        if (llen > PRIO_LEN)
            llen = PRIO_LEN;

        if (this->llen != llen) {
            this->llen = llen;

            if (tstack_storage_not_empty())
                std::cout << "[vp::aplic-uia] Warning: HW priority levels stack is not empty when LLEN changes" << std::endl;
        }
    }

    void set_primary(bool primary) {
        if (this->primary != primary) {
            this->primary = primary;

            if (tstack_storage_not_empty())
                std::cout << "[vp::aplic-uia] Warning: HW priority levels stack is not empty when PRIMARY mode changes" << std::endl;
        }
    }

    bool is_masked_by_threshold(struct composite_prio prio) const {
        if (prio.is_masked_by_threshold(sw_threshold))
            return true;

        if (primary && !can_preempt(prio))
            return true;

        return false;
    }

    uint32_t to_ithreshold(void) {
        return sw_threshold | (sw_prev_threshold << APLIC_ITHRESHOLD_PTRSH_SHIFT);
    }

    uint32_t to_llen(void) const {
        return llen;
    }

    void do_claim(struct uia_interrupt irq) {
        assert(irq.iid < NumberInterrupts);

        // HW priority levels stack management is only in primary mode
        if (!primary)
            return;

        if (irq.iid != 0) {
            // claim
            tstack_insert(prio_to_level(irq.prio));
        }
    }

    void do_complete(void) {
        // HW priority levels stack management is only in primary mode
        if (!primary)
            return;

        tstack_pop();
    }

    void do_claim_complete(struct uia_interrupt irq) {
        assert(irq.iid < NumberInterrupts);

        bool claim = irq.iid != 0;

        // SW previous threshold is updated in all modes (on claim only)
        if (claim)
            sw_prev_threshold = sw_threshold;

        // HW priority levels stack management is only in primary mode
        if (!primary)
            return;

        if (claim) {
            // claim
            tstack_insert(prio_to_level(irq.prio));
        } else {
            // complete
            tstack_pop();
        }
    }

    bool is_in_irq_context() const {
        if (!primary)
            return false;

        return tstack_curr_llen_has_entries();
    }

    uint32_t tstack_get_current_level(void) const {
        for (unsigned i = 0; i < llen_to_levels(llen); i++) {
            // Start from the lowest level (highest priority)
            if (thresholds[i]) {
                return i;
            }
        }

        return NO_LEVELS; // empty
    }

private:
    bool can_preempt(struct composite_prio prio) const {
        if (!primary)
            return false;

        uint32_t current_level = tstack_get_current_level();
        uint32_t irq_level = prio_to_level(prio);

        if (trace_mode) {
            std::cout << "[vp::aplic-uia] Checking preemption: IRQ level=" << static_cast<uint32_t>(irq_level) <<
                ", IRQ prio=" << prio;
            if (current_level == NO_LEVELS)
                std::cout << ",Current level=empty" << std::endl;
            else
                std::cout << ",Current level=" << static_cast<uint32_t>(current_level) << std::endl;

        }

        return irq_level < current_level;
    }

    bool tstack_has_entries(uint32_t upper_limit) const {
        for (unsigned i = 0; i < upper_limit; i++) {
            if (thresholds[i])
                return true;
        }

        return false;
    }

    // for debug purposes only
    bool tstack_storage_not_empty(void) const {
        return tstack_has_entries(MAX_LEVELS);
    }

    bool tstack_curr_llen_has_entries(void) const {
        return tstack_has_entries(llen_to_levels(llen));
    }

    void tstack_pop(void) {
        for (unsigned i = 0; i < llen_to_levels(llen); i++) {
            // Start from the lowest level (highest priority)
            if (thresholds[i]) {
                thresholds[i] = false;

                if (trace_mode)
                    std::cout << "[vp::aplic-uia] HW prio levels: stack pop level " << static_cast<uint32_t>(i) << std::endl;

                return;
            }
        }
    }

    void tstack_insert(uint8_t level) {
        assert(level < MAX_LEVELS);

        thresholds[level] = true;

        if (trace_mode)
            std::cout << "[vp::aplic-uia] HW prio levels: stack insert level " << static_cast<uint32_t>(level) << std::endl;
    }

    static constexpr uint32_t llen_to_levels(uint8_t llen) {
        return 1 << llen;
    }

    uint8_t prio_to_level(struct composite_prio prio) const {
        if (llen == 0) {
            // No priority levels, all interrupts have the same level
            return 0;
        }

        uint8_t level_shift = PRIO_LEN - llen + 1;
        uint8_t level_val = (prio.to_prio() & GENMASK(PRIO_LEN, level_shift)) >> level_shift;

        return level_val;
    }

    uint32_t sw_threshold = 0, sw_prev_threshold = 0;
    uint32_t llen = 0;
    bool primary = false;

    // llen = 7, PRIO_LEN = 7
    static constexpr uint32_t MAX_LEVELS = 128;
    static_assert(MAX_LEVELS == llen_to_levels(PRIO_LEN));
    static constexpr uint32_t NO_LEVELS = MAX_LEVELS + 1;

    bool thresholds[MAX_LEVELS + 1] = {};
};


template <unsigned NumberCores, unsigned NumberDomains, unsigned NumberInterrupts>
struct APLIC_UIA : public sc_core::sc_module, public interrupt_gateway, public primary_interrupt_controller_if {
    static_assert(NumberInterrupts <= 63, "out of bound");
    static_assert(NumberDomains > 0 && NumberDomains <= 2, "out of bound");
    static_assert(NumberCores == 1, "APLIC_UIA supports only single core");

    tlm_utils::simple_target_socket<APLIC_UIA> tsock;
    tlm_utils::simple_initiator_socket<APLIC_UIA> isock;

    std::array<external_interrupt_target *, NumberCores> target_harts{};

    APLIC_UIA_DOMAIN domains[NumberDomains] = {APLIC_UIA_DOMAIN(APLIC_M_DOMAIN), APLIC_UIA_DOMAIN(APLIC_S_DOMAIN)};
    std::vector<RegisterRange *> register_ranges;

    sc_core::sc_event e_run;
    sc_core::sc_time clock_cycle;

    bitset<NumberInterrupts + 1> astate_ie;
    bitset<NumberInterrupts + 1> astate_ip;
    // level storage for level-triggered interrupts
    bitset<NumberInterrupts + 1> astate_input;
    bitset<NumberInterrupts + 1> astate_deleg;
    struct source_mode astate_sm[NumberInterrupts + 1] = {};
    struct composite_prio astate_prio[NumberInterrupts + 1] = {};
    bool astate_domain_enabled[NumberDomains] = {};
    bool astate_delivery_enabled[NumberDomains] = {};
    struct threshold_mngr<NumberInterrupts> astate_threshold_mngr[NumberDomains] = {};
    bool astate_primary = false;

    bool trace_mode = false;

    SC_HAS_PROCESS(APLIC_UIA);

    APLIC_UIA(sc_core::sc_module_name, bool trace_mode = false) : trace_mode(trace_mode) {
        for (unsigned i = 0; i < NumberDomains; i++) {
            astate_threshold_mngr[i].trace_mode = trace_mode;
            domains[i].trace_mode = trace_mode;

            domains[i].regs_domaincfg->post_write_callback = std::bind(&APLIC_UIA::post_write_domainconfig, this, std::placeholders::_1, i);
            *domains[i].domaincfg = domaincfg_mask(0);
            domains[i].regs_sourcecfg->post_write_callback = std::bind(&APLIC_UIA::post_write_sourcecfg, this, std::placeholders::_1, i);
            domains[i].regs_sourcecfg->pre_read_callback = std::bind(&APLIC_UIA::pre_read_sourcecfg, this, std::placeholders::_1, i);

            domains[i].regs_setipnum->post_write_callback = std::bind(&APLIC_UIA::post_write_bitixnum, this, std::placeholders::_1, std::ref(astate_ip), true, i);
            domains[i].regs_clripnum->post_write_callback = std::bind(&APLIC_UIA::post_write_bitixnum, this, std::placeholders::_1, std::ref(astate_ip), false, i);
            domains[i].regs_setienum->post_write_callback = std::bind(&APLIC_UIA::post_write_bitixnum, this, std::placeholders::_1, std::ref(astate_ie), true, i);
            domains[i].regs_clrienum->post_write_callback = std::bind(&APLIC_UIA::post_write_bitixnum, this, std::placeholders::_1, std::ref(astate_ie), false, i);

            // IDC structure
            domains[i].regs_idelivery->post_write_callback = std::bind(&APLIC_UIA::post_write_idelivery, this, std::placeholders::_1, i);
            domains[i].regs_topi->pre_read_callback = std::bind(&APLIC_UIA::pre_read_topi, this, std::placeholders::_1, i);
            domains[i].regs_claimi->pre_read_callback = std::bind(&APLIC_UIA::pre_read_claimi, this, std::placeholders::_1, i);
            domains[i].regs_ithreshold->post_write_callback = std::bind(&APLIC_UIA::post_write_ithreshold, this, std::placeholders::_1, i);
            domains[i].regs_ithreshold->pre_read_callback = std::bind(&APLIC_UIA::pre_read_ithreshold, this, std::placeholders::_1, i);

            // Temporary extra registers
            domains[i].regs_only_complete->pre_read_callback = std::bind(&APLIC_UIA::pre_read_only_complete, this, std::placeholders::_1, i);
            domains[i].regs_only_claim->pre_read_callback = std::bind(&APLIC_UIA::pre_read_only_claim, this, std::placeholders::_1, i);
            domains[i].regs_debug_hw_threshold->pre_read_callback = std::bind(&APLIC_UIA::pre_read_debug_hw_threshold, this, std::placeholders::_1, i);

            domains[i].map_register_ranges(register_ranges);
        }

        clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
        tsock.register_b_transport(this, &APLIC_UIA::transport);

        SC_THREAD(run);
    }

    void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
        delay += 4 * clock_cycle;
        vp::mm::route("APLIC_UIA", register_ranges, trans, delay);
    }

    static uint32_t domaincfg_mask(uint32_t val) {
        uint32_t domaincfg_reg = 0;

        domaincfg_reg &= ~APLIC_DOMAINCFG_BE_BIT; /* Always little endian */
        domaincfg_reg &= ~APLIC_DOMAINCFG_DM_BIT; /* Only direct delivery mode support */
        domaincfg_reg |= val & APLIC_DOMAINCFG_IE_BIT;
        domaincfg_reg |= 0x80 << APLIC_DOMAINCFG_RO80_SHIFT;

        return domaincfg_reg;
    }

    void post_write_domainconfig(RegisterRange::WriteInfo t, unsigned domain) {
        uint32_t *domaincfg = reinterpret_cast<uint32_t *>(t.var_addr);

        uint32_t val = domaincfg_mask(*domaincfg);
        astate_domain_enabled[domain] = !!(val & APLIC_DOMAINCFG_IE_BIT);
        *domaincfg = val;

        if (trace_mode)
            std::cout << "[vp::aplic-uia] W access to domainconfig, EN=" <<
                (astate_domain_enabled[domain] ? 1 : 0) <<" (domain = " << domain << ")" << std::endl;

        e_run.notify(clock_cycle);
    }

    bool pre_read_sourcecfg(RegisterRange::ReadInfo t, unsigned domain)
    {
        uint32_t *sourcecfg = reinterpret_cast<uint32_t *>(t.var_addr);
        uint32_t iid = (t.addr >> 2) + 1; // 1-based iid

        if (iid > NumberInterrupts) {
            *sourcecfg = 0;
        } else if (astate_deleg.test(iid) && domain == APLIC_M_DOMAIN) {
            // Child index is read-only zero (single child only)
            // Only delegation bit present
            *sourcecfg = APLIC_SOURCECFG_D_BIT;
        } else if (!astate_deleg.test(iid) && domain == APLIC_S_DOMAIN) {
            // Not accessible in current domain => sourcecfg = 0
            *sourcecfg = 0;
        } else {
            *sourcecfg = astate_ie.test(iid) << APLIC_SOURCECFG_IE_SHIFT |
                         astate_ip.test(iid) << APLIC_SOURCECFG_IP_SHIFT |
                         // D bit is always zero here
                         astate_prio[iid].to_sourcecfg_prio() << APLIC_SOURCECFG_IPRIO_SHIFT |
                         astate_sm[iid].mode << APLIC_SOURCECFG_SM_SHIFT;
        }

        return true;
    }

    uia_interrupt get_top_interrupt(int domain) {
        if (!astate_domain_enabled[domain] || !astate_delivery_enabled[domain])
            return uia_interrupt(); // All interrupts are disabled

        bitset<NumberInterrupts + 1> pending = astate_ip & astate_ie;

        if (domain == APLIC_M_DOMAIN)
            pending &= ~astate_deleg;
        else
            pending &= astate_deleg;

        if (pending.none())
            return uia_interrupt(); // No pending interrupts

        // Find the topi for current domain
        uint32_t iid = 0;
        struct composite_prio prio;

        for (unsigned i = 1; i <= NumberInterrupts; i++) {
            if (pending.test(i) && astate_prio[i].is_valid() && astate_prio[i] < prio) {
                iid = i;
                prio = astate_prio[i];
            }
        }

        assert(iid != 0 && prio.is_valid());

        if (astate_threshold_mngr[domain].is_masked_by_threshold(prio))
            return uia_interrupt(); // No interrupt above threshold

        return uia_interrupt(iid, prio);
    }

    bool pre_read_topi(RegisterRange::ReadInfo t, unsigned domain) {
        uint32_t *topi = reinterpret_cast<uint32_t *>(t.var_addr);

        struct uia_interrupt irq = get_top_interrupt(domain);
        *topi = irq.to_topi();

        if (trace_mode)
            std::cout << "[vp::aplic-uia] R access to topi, " << irq <<
                " (domain = " << domain << ")" << std::endl;

        return true;
    }

    void claim_interrupt(struct uia_interrupt irq, unsigned domain) {
        if (trace_mode)
            std::cout << "[vp::aplic-uia] claimed " << irq <<
                " (domain = " << domain << ")" << std::endl;

        // TODO: do we need to do e_run.notify(clock_cycle); here?

        if (irq.iid < 1 || irq.iid >= NumberInterrupts)
            return;

        /* Level-triggered interrupts cannot be set/cleared by software */
        if (!astate_sm[irq.iid].is_level())
            astate_ip.reset(irq.iid);

        e_run.notify(clock_cycle);
    }

    bool pre_read_claimi(RegisterRange::ReadInfo t, unsigned domain) {
        uint32_t *claimi = reinterpret_cast<uint32_t *>(t.var_addr);

        struct uia_interrupt irq = get_top_interrupt(domain);
        *claimi = irq.to_topi();

        // if (trace_mode)  std::cout << "[vp::aplic-uia] R access to domain " << domain << " claimi = 0x" << std::hex << *claimi << std::endl;

        astate_threshold_mngr[domain].do_claim_complete(irq);
        claim_interrupt(irq, domain);

        return true;
    }

    bool pre_read_only_claim(RegisterRange::ReadInfo t, unsigned domain) {
        uint32_t *only_claim = reinterpret_cast<uint32_t *>(t.var_addr);

        struct uia_interrupt irq = get_top_interrupt(domain);
        *only_claim = irq.to_topi();

        astate_threshold_mngr[domain].do_claim(irq);
        claim_interrupt(irq, domain);

        return true;
    }

    bool pre_read_only_complete(RegisterRange::ReadInfo t, unsigned domain) {
        uint32_t *only_complete = reinterpret_cast<uint32_t *>(t.var_addr);
        *only_complete = 0;

        astate_threshold_mngr[domain].do_complete();

        if (trace_mode)
            std::cout << "[vp::aplic-uia] R access to only complete (domain = " << domain << ")" << std::endl;

        return true;
    }

    bool pre_read_ithreshold(RegisterRange::ReadInfo t, unsigned domain) {
        uint32_t *ithreshold = reinterpret_cast<uint32_t *>(t.var_addr);

        *ithreshold = astate_threshold_mngr[domain].to_ithreshold();

        return true;
    }

    bool pre_read_debug_hw_threshold(RegisterRange::ReadInfo t, unsigned domain) {
        uint32_t *debug_hw_threshold = reinterpret_cast<uint32_t *>(t.var_addr);

        *debug_hw_threshold = astate_threshold_mngr[domain].tstack_get_current_level();

        return true;
    }

    void post_write_ithreshold(RegisterRange::WriteInfo t, unsigned domain) {
        uint32_t *ithreshold = reinterpret_cast<uint32_t *>(t.var_addr);

        if (trace_mode)
            std::cout << "[vp::aplic-uia] set threshold to " << std::dec << *ithreshold <<
                " (domain = " << domain << ")" << std::endl;

        astate_threshold_mngr[domain].threshold_from_ithreshold(*ithreshold);
        e_run.notify(clock_cycle);
    }

    void post_write_idelivery(RegisterRange::WriteInfo t, unsigned domain) {
        uint32_t *idelivery = reinterpret_cast<uint32_t *>(t.var_addr);
        uint32_t val = *idelivery & APLIC_IDELIVERY_RW_MASK;

        astate_delivery_enabled[domain] = !!(val & APLIC_IDELIVERY_ENABLE_BIT);

        // TODO: FIXME: check the spec logic regarding the primary bit in S-domain
        // Primary bit is read-write in M-domain
        if (domain == APLIC_M_DOMAIN)
            astate_primary = !!(val & APLIC_IDELIVERY_PRIMARY_BIT);
        else
            val &= ~APLIC_IDELIVERY_PRIMARY_BIT;

        astate_threshold_mngr[domain].llen_from_idelivery(val);
        astate_threshold_mngr[APLIC_M_DOMAIN].set_primary(astate_primary);
        astate_threshold_mngr[APLIC_S_DOMAIN].set_primary(astate_primary);

        if (trace_mode)
            std::cout << "[vp::aplic-uia] set idelivery to 0x" << std::hex << *idelivery << " (primary=" <<
                astate_primary << ", enabled=" << astate_delivery_enabled[domain] << ", llen=" <<
                astate_threshold_mngr[domain].to_llen() << ") (domain = " << domain << ")" << std::endl;

        e_run.notify(clock_cycle);
    }

    void post_write_sourcecfg(RegisterRange::WriteInfo t, unsigned domain) {
        uint32_t *sourcecfg = reinterpret_cast<uint32_t *>(t.var_addr);
        uint32_t iid = (t.addr >> 2) + 1;
        uint32_t sourcecfg_val = *sourcecfg;

        assert(iid > 0 && iid < 1024);

        // If source is not implemented => sourcecfg = 0
        if (iid > NumberInterrupts)
            return;

        bool is_accessible_in_domain = true;
        bool is_delegated = false;

        if (domain == APLIC_M_DOMAIN) {
            // APLIC_SOURCECFG_D_BIT is writeable and can change the sourcecfg layout
            is_delegated = sourcecfg_val & APLIC_SOURCECFG_D_BIT;
            astate_deleg.set(iid, is_delegated);
        } else {
            assert(domain - 1 == APLIC_M_DOMAIN);
            is_accessible_in_domain = astate_deleg.test(iid);

            is_delegated = false;
        }

        // Not accessible in current domain => sourcecfg = 0
        if (!is_accessible_in_domain)
            return;

        if (is_delegated) {
            if (trace_mode)
                std::cout << "[vp::aplic-uia] sourcecfg iid=" << std::dec << iid <<
                    " delegated, (domain = " << domain << ")" << std::endl;

            e_run.notify(clock_cycle);

            return;
        }

        // TODO: FIXME: align sourcecfg SM transition behavior with the spec:
        // Any write to a sourcecfg register might (or might not) cause the corresponding interrupt-pending bit
        // to be set to one if the rectified input value is high (= 1) under the new source mode. A write to a
        // sourcecfg register will not by itself cause a pending bit to be cleared except when the source is made
        // inactive.

        // Main sourcecfg layout
        astate_sm[iid].from_sourcecfg(sourcecfg_val);

        if (astate_sm[iid].is_inactive()) {
            // For an inactive source, interrupt-pending and interrupt-enable bits are read-only zeros.
            astate_ie.reset(iid);
            astate_ip.reset(iid);
            astate_prio[iid] = composite_prio();
        } else {
            // obtain priority for active sources
            astate_prio[iid].from_sourcecfg(sourcecfg_val);

            // for level-triggered source: sync pending bit with input value
            // NOTE: check for SM==LEVELx is performed in level_irq_sync_ip_to_input()
            level_irq_sync_ip_to_input(iid);
        }

        e_run.notify(clock_cycle);
    }

    void post_write_bitixnum(RegisterRange::WriteInfo t, bitset<NumberInterrupts + 1> &astate, bool set, unsigned domain) {
        uint32_t *reg = reinterpret_cast<uint32_t *>(t.var_addr);
        uint32_t iid = *reg;

        if (trace_mode)
            std::cout << "[vp::aplic-uia] attempt " << ((&astate == &astate_ip) ? "pending" : "enable") << " bit " <<
                (set ? "set" : "clear") << " iid=" << std::dec << iid << " (domain = " << domain << ")" << std::endl;

        *reg = 0; /* A read of setixnum/clrixnum registers always returns zero */

        // iid bounds checked in is_source_active()
        if (!is_source_active(domain, iid))
            return;

        /* Level-triggered interrupts cannot be set/cleared by software */
        if ((&astate == &astate_ip) && astate_sm[iid].is_level())
            return;

        astate.set(iid, set);

        e_run.notify(clock_cycle); /* We have set/cleared a pending/enable bit - we need to notify core */
    }

    void set_input(uint32_t iid, bool input_value, bool input_level_type) {
        if (iid < 1 || iid >= NumberInterrupts)
            return;

        bool previous_input_value = astate_input.test(iid);

        if (trace_mode)
            std::cout << "[vp::aplic-uia] route irq iid=" << std::dec << iid << ", sm=" <<
                astate_sm[iid] << ", input type=" << (input_level_type ? "level" : "edge") <<
                ", input value=" << (previous_input_value ? "1" : "0") << "->" << (input_value ? "1" : "0") <<
                " to APLIC" << std::endl;

        if (input_level_type) {
            astate_input.set(iid, input_value);

            if (astate_sm[iid].is_level()) {
                level_irq_sync_ip_to_input(iid);
                e_run.notify(clock_cycle);
            } else if (astate_sm[iid].mode == source_mode::EDGE0 && previous_input_value && !input_value) {
                // Falling edge
                astate_ip.set(iid);
                e_run.notify(clock_cycle);
            } else if (astate_sm[iid].mode == source_mode::EDGE1 && !previous_input_value && input_value) {
                // Rising edge
                astate_ip.set(iid);
                e_run.notify(clock_cycle);
            }
        } else {
            // treat all edge sources as producing _|""|_ edges - so they trigger both on rising (EDGE1)
            // and falling (EDGE0) interrupt lines

            astate_input.set(iid, 0);

            if (astate_sm[iid].is_edge()) {
                astate_ip.set(iid);
                e_run.notify(clock_cycle);
            }
        }
    }

    void level_irq_sync_ip_to_input(uint32_t iid) {
        if (iid < 1 || iid >= NumberInterrupts)
            return;

        if (astate_sm[iid].mode == source_mode::LEVEL1)
            astate_ip.set(iid, astate_input.test(iid));
        else if (astate_sm[iid].mode == source_mode::LEVEL0)
            astate_ip.set(iid, !astate_input.test(iid));
        else
            return;
    }

    bool is_source_active(int domain, uint32_t iid) {
        if (iid < 1 || iid >= NumberInterrupts)
            return false;

        // M-domain && delegated || S-domain && not delegated
        if (astate_deleg.test(iid) != !!domain)
            return false;

        if (astate_sm[iid].is_inactive())
            return false;

        return true;
    }

    bool is_pending_in_domain(int domain) {
        struct uia_interrupt irq = get_top_interrupt(domain);

        return irq.iid != 0;
    }

    // interrupt_gateway interface
    void gateway_trigger_interrupt(uint32_t iid) override {
        set_input(iid, true, false);
    }

    // primary_interrupt_controller_if interface
    bool is_primary() override {
        return astate_primary;
    }

    bool is_pending(PrivilegeLevel level) override {
        if (level != MachineMode && level != SupervisorMode)
            return false;

        return is_pending_in_domain((level == MachineMode) ? APLIC_M_DOMAIN : APLIC_S_DOMAIN);
    }

    bool is_in_irq_context(PrivilegeLevel level) override {
        if (level != MachineMode && level != SupervisorMode)
            return false;

        return astate_threshold_mngr[(level == MachineMode) ? APLIC_M_DOMAIN : APLIC_S_DOMAIN].is_in_irq_context();
    }

    // clint_interrupt_if interface
    void trigger_timer_interrupt(bool status, PrivilegeLevel timer) override {
        if (!astate_primary)
            return;

        if (timer == MachineMode)
            set_input(EXC_M_TIMER_INTERRUPT, status, true);
        else if (timer == SupervisorMode) // TODO: do we need to check for csrs.menvcfgh.is_timer_enabled() here as well?
            set_input(EXC_S_TIMER_INTERRUPT, status, true);
    }

    void trigger_software_interrupt(bool status, PrivilegeLevel sw_irq_type) override {
        if (!astate_primary)
            return;

        if (sw_irq_type == MachineMode)
            set_input(EXC_M_SOFTWARE_INTERRUPT, status, true);
        else if (sw_irq_type == SupervisorMode && status)
            // NOTE: S software interrupt is edge-triggered
            set_input(EXC_S_SOFTWARE_INTERRUPT, status, false);
    }


    void run() {
        while (true) {
            sc_core::wait(e_run);

            if (!astate_primary) {
                if (is_pending_in_domain(APLIC_M_DOMAIN)) {
                    if (trace_mode) std::cout << "[vp::aplic-uia] external irq for M-mode: assert" << std::endl;
                    target_harts[HART_0_INDEX]->trigger_external_interrupt(MachineMode);
                } else {
                    if (trace_mode) std::cout << "[vp::aplic-uia] external irq for M-mode: deassert" << std::endl;
                    target_harts[HART_0_INDEX]->clear_external_interrupt(MachineMode);
                }

                if (is_pending_in_domain(APLIC_S_DOMAIN)) {
                    if (trace_mode) std::cout << "[vp::aplic-uia] external irq for S-mode: assert" << std::endl;
                    target_harts[HART_0_INDEX]->trigger_external_interrupt(SupervisorMode);
                } else {
                    if (trace_mode) std::cout << "[vp::aplic-uia] external irq for S-mode: deassert" << std::endl;
                    target_harts[HART_0_INDEX]->clear_external_interrupt(SupervisorMode);
                }
            }
        }
    }
};
