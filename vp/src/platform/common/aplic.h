#pragma once

#include <tlm_utils/simple_target_socket.h>
#include <systemc>

#include "core/common/irq_if.h"
#include "util/memory_map.h"
#include "util/tlm_map.h"


static constexpr bool trace_mode = false;

#define IMSIC_HART0_BASE          0x31000000
/* Address to deliver M-mode MSI, align:4096 */
#define IMSIC_HART0_M_IFILE       (IMSIC_HART0_BASE + 0)
/* Address to deliver S-mode MSI, align:4096 */
#define IMSIC_HART0_S_IFILE       (IMSIC_HART0_BASE + 4096)


#define HART_0_INDEX              (0)	/* Only one hart now */

#define APLIC_BASE                (0x40000000)
#define APLIC_DOMAIN_OFFSET       (0x10000)

#define APLIC_M_DOMAIN            (0)
#define APLIC_S_DOMAIN            (1)

#define APLIC_PAGE_BITS           12
#define APLIC_PAGE_SIZE           (1 << APLIC_PAGE_BITS)

#define APLIC_DOMAINCFG           (0x0000)
#define APLIC_SOURCECFG           (0x0004)
#define APLIC_MMSIADDRCFG         (0x1BC0)
#define APLIC_MMSIADDRCFGH        (0x1BC4)
#define APLIC_SMSIADDRCFG         (0x1BC8)
#define APLIC_SMSIADDRCFGH        (0x1BCC)
#define APLIC_SETIP               (0x1C00)
#define APLIC_SETIPNUM            (0x1CDC)
#define APLIC_IN_CLRIP            (0x1D00)
#define APLIC_CLRIPNUM            (0x1DDC)
#define APLIC_SETIE               (0x1E00)
#define APLIC_SETIENUM            (0x1EDC)
#define APLIC_CLRIE               (0x1F00)
#define APLIC_CLRIENUM            (0x1FDC)
#define APLIC_SETIPNUMLE          (0x2000)
#define APLIC_SETIPNUMBE          (0x2004)
#define APLIC_GENMSI              (0x3000)
#define APLIC_TARGET              (0x3004)

#define BIT(n)  (1ULL << (n))

/* Bit definitions */
#define APLIC_DOMAINCFG_BE_BIT              0ULL
#define APLIC_DOMAINCFG_BE_MASK             0x1
#define APLIC_DOMAINCFG_DM_BIT              2ULL
#define APLIC_DOMAINCFG_DM_MASK             0x1
#define APLIC_DOMAINCFG_IE_BIT              8ULL
#define APLIC_DOMAINCFG_IE_MASK             0x1
#define APLIC_DOMAINCFG_RO80_BIT            24ULL
#define APLIC_DOMAINCFG_RO80_MASK           0xFF

#define APLIC_SOURCECFG_CHILDIND_BIT        0ULL
#define APLIC_SOURCECFG_CHILDIND_MASK       0x3FF
#define APLIC_SOURCECFG_SM_BIT              0ULL
#define APLIC_SOURCECFG_SM_MASK             0x7
#define APLIC_SOURCECFG_D_BIT               10ULL
#define APLIC_SOURCECFG_D_MASK              0x1

enum source_mode {
	INACTIVE = 0, /* Inactive in this domain (and not delegated) */
	DETACHED = 1, /* Active, detached from the source wire */
	RESERVED1 = 2,
	RESERVED2 = 3,
	EDGE1 = 4, /* Active, edge-sensitive; interrupt asserted on rising edge */
	EDGE0 = 5, /* Active, edge-sensitive; interrupt asserted on falling edge */
	LEVEL1 = 6, /* Active, level-sensitive; interrupt asserted when high */
	LEVEL0 = 7 /* Active, level-sensitive; interrupt asserted when low */
};

#define APLIC_MMSIADDRCFGH_HIPPN_BIT        0ULL
#define APLIC_MMSIADDRCFGH_HIPPN_MASK       0xFFF
#define APLIC_MMSIADDRCFGH_LHXW_BIT         12ULL
#define APLIC_MMSIADDRCFGH_LHXW_MASK        0xF
#define APLIC_MMSIADDRCFGH_HHXW_BIT         16ULL
#define APLIC_MMSIADDRCFGH_HHXW_MASK        0x7
#define APLIC_MMSIADDRCFGH_LHXS_BIT         20ULL
#define APLIC_MMSIADDRCFGH_LHXS_MASK        0x7
#define APLIC_MMSIADDRCFGH_HHXS_BIT         24ULL
#define APLIC_MMSIADDRCFGH_HHXS_MASK        0x1F

#define APLIC_SMSIADDRCFGH_HIPPN_BIT        0ULL
#define APLIC_SMSIADDRCFGH_HIPPN_MASK       0xFFF
#define APLIC_SMSIADDRCFGH_LHXS_BIT         20ULL
#define APLIC_SMSIADDRCFGH_LHXS_MASK        0x1FFF

#define APLIC_GENMSI_EIID_BIT               0ULL
#define APLIC_GENMSI_EIID_MASK              0x7FF
#define APLIC_GENMSI_MSIDELIV_BIT           11ULL
#define APLIC_GENMSI_MSIDELIV_MASK          0x1
#define APLIC_GENMSI_BUSY_BIT               12ULL
#define APLIC_GENMSI_BUSY_MASK              0x1
#define APLIC_GENMSI_CONTEXT_BIT            13ULL
#define APLIC_GENMSI_CONTEXT_MASK           0x1F
#define APLIC_GENMSI_HARTIND_BIT            18ULL
#define APLIC_GENMSI_HARTIND_MASK           0x3FFF

#define APLIC_TARGETS_EIID_BIT              0ULL
#define APLIC_TARGETS_EIID_MASK             0x7FF
#define APLIC_TARGETS_MSIDELIV_BIT          11ULL
#define APLIC_TARGETS_MSIDELIV_MASK         0x1
#define APLIC_TARGETS_DMSICONTX_BIT         12ULL
#define APLIC_TARGETS_DMSICONTX_MASK        0x3F
#define APLIC_TARGETS_HARTIND_BIT           18ULL
#define APLIC_TARGETS_HARTIND_MASK          0x3FFF


template <unsigned NumberCores, unsigned NumberDomains, unsigned NumberInterrupts, unsigned NumberInterruptEntries, uint32_t MaxPriority>
struct APLIC : public sc_core::sc_module, public interrupt_gateway {
	static_assert(NumberInterrupts <= 1024, "out of bound");
	static_assert(NumberCores <= 15360, "out of bound");
	static constexpr unsigned WORDS_FOR_INTERRUPT_ENTRIES = (NumberInterruptEntries+(32-1))/32;

	tlm_utils::simple_target_socket<APLIC> tsock;
	tlm_utils::simple_initiator_socket<APLIC> isock;

	std::array<external_interrupt_target *, NumberCores> target_harts{};

	/* APLIC memory-mapped control region (for each interrupt domain that an APLIC supports) */
	RegisterRange *regs_domaincfg[NumberDomains];
	IntegerView<uint32_t> *domaincfg[NumberDomains];

	RegisterRange *regs_sourcecfg[NumberDomains];
	ArrayView<uint32_t> *sourcecfg[NumberDomains];

	RegisterRange *regs_mmsiaddrcfg[NumberDomains];
	IntegerView<uint32_t> *mmsiaddrcfg[NumberDomains];

	RegisterRange *regs_mmsiaddrcfgh[NumberDomains];
	IntegerView<uint32_t> *mmsiaddrcfgh[NumberDomains];

	RegisterRange *regs_smsiaddrcfg[NumberDomains];
	IntegerView<uint32_t> *smsiaddrcfg[NumberDomains];

	RegisterRange *regs_smsiaddrcfgh[NumberDomains];
	IntegerView<uint32_t> *smsiaddrcfgh[NumberDomains];

	RegisterRange *regs_setip[NumberDomains];
	ArrayView<uint32_t> *setip[NumberDomains];

	RegisterRange *regs_setipnum[NumberDomains];
	IntegerView<uint32_t> *setipnum[NumberDomains];

	RegisterRange *regs_in_clrip[NumberDomains];
	ArrayView<uint32_t> *in_clrip[NumberDomains];

	RegisterRange *regs_clripnum[NumberDomains];
	IntegerView<uint32_t> *clripnum[NumberDomains];

	RegisterRange *regs_setie[NumberDomains];
	ArrayView<uint32_t> *setie[NumberDomains];

	RegisterRange *regs_setienum[NumberDomains];
	IntegerView<uint32_t> *setienum[NumberDomains];

	RegisterRange *regs_clrie[NumberDomains];
	ArrayView<uint32_t> *clrie[NumberDomains];

	RegisterRange *regs_clrienum[NumberDomains];
	IntegerView<uint32_t> *clrienum[NumberDomains];

	RegisterRange *regs_setipnum_le[NumberDomains];
	IntegerView<uint32_t> *setipnum_le[NumberDomains];

	RegisterRange *regs_setipnum_be[NumberDomains];
	IntegerView<uint32_t> *setipnum_be[NumberDomains];

	RegisterRange *regs_genmsi[NumberDomains];
	IntegerView<uint32_t> *genmsi[NumberDomains];

	RegisterRange *regs_target[NumberDomains];
	ArrayView<uint32_t> *target[NumberDomains];

	std::vector<RegisterRange *> register_ranges;

	sc_core::sc_event e_run;
	sc_core::sc_time clock_cycle;

	uint32_t ie_reg[NumberDomains][32];
	uint32_t ip_reg[NumberDomains][32];

	SC_HAS_PROCESS(APLIC);

	APLIC(sc_core::sc_module_name, PrivilegeLevel level = MachineMode) {
		for (unsigned ii = 0; ii < NumberDomains; ii++) {
			uint32_t domain_base = APLIC_DOMAIN_OFFSET * ii;

			regs_domaincfg[ii] = new RegisterRange{domain_base + APLIC_DOMAINCFG, sizeof(uint32_t) * 1};
			domaincfg[ii] = new IntegerView<uint32_t>(*regs_domaincfg[ii]);
			(*regs_domaincfg[ii]).post_write_callback = std::bind(&APLIC::post_write_domainconfig, this, std::placeholders::_1);

			regs_sourcecfg[ii] = new RegisterRange{domain_base + APLIC_SOURCECFG, sizeof(uint32_t) * NumberInterrupts};
			sourcecfg[ii] = new ArrayView<uint32_t>(*regs_sourcecfg[ii]);
			(*regs_sourcecfg[ii]).post_write_callback = std::bind(&APLIC::post_write_sourcecfg, this, std::placeholders::_1);

			/* Implemented only at the M-mode */
			regs_mmsiaddrcfg[ii] = new RegisterRange{domain_base + APLIC_MMSIADDRCFG, sizeof(uint32_t) * 1};
			mmsiaddrcfg[ii] = new IntegerView<uint32_t>(*regs_mmsiaddrcfg[ii]);

			/* Implemented only at the M-mode */
			regs_mmsiaddrcfgh[ii] = new RegisterRange{domain_base + APLIC_MMSIADDRCFGH, sizeof(uint32_t) * 1};
			mmsiaddrcfgh[ii] = new IntegerView<uint32_t>(*regs_mmsiaddrcfgh[ii]);

			/* Implemented only at the M-mode */
			regs_smsiaddrcfg[ii] = new RegisterRange{domain_base + APLIC_SMSIADDRCFG, sizeof(uint32_t) * 1};
			smsiaddrcfg[ii] = new IntegerView<uint32_t>(*regs_smsiaddrcfg[ii]);

			/* Implemented only at the M-mode */
			regs_smsiaddrcfgh[ii] = new RegisterRange{domain_base + APLIC_SMSIADDRCFGH, sizeof(uint32_t) * 1};
			smsiaddrcfgh[ii] = new IntegerView<uint32_t>(*regs_smsiaddrcfgh[ii]);

			if (ii != APLIC_M_DOMAIN) { /* Machine domain only */
				(*regs_mmsiaddrcfg[ii]).post_write_callback = std::bind(&APLIC::post_write_xmsiaddrcfg, this, std::placeholders::_1);
				(*regs_mmsiaddrcfgh[ii]).post_write_callback = std::bind(&APLIC::post_write_xmsiaddrcfg, this, std::placeholders::_1);
				(*regs_smsiaddrcfg[ii]).post_write_callback = std::bind(&APLIC::post_write_xmsiaddrcfg, this, std::placeholders::_1);
				(*regs_smsiaddrcfgh[ii]).post_write_callback = std::bind(&APLIC::post_write_xmsiaddrcfg, this, std::placeholders::_1);
			}

			regs_setip[ii] = new RegisterRange{domain_base + APLIC_SETIP, sizeof(uint32_t) * 32};
			setip[ii] = new ArrayView<uint32_t>(*regs_setip[ii]);
			(*regs_setip[ii]).post_write_callback = std::bind(&APLIC::post_write_setip, this, std::placeholders::_1);
			(*regs_setip[ii]).pre_read_callback = std::bind(&APLIC::pre_read_setip, this, std::placeholders::_1);

			regs_setipnum[ii] = new RegisterRange{domain_base + APLIC_SETIPNUM, sizeof(uint32_t) * 1};
			setipnum[ii] = new IntegerView<uint32_t>(*regs_setipnum[ii]);
			(*regs_setipnum[ii]).post_write_callback = std::bind(&APLIC::post_write_setipnum, this, std::placeholders::_1);

			regs_in_clrip[ii] = new RegisterRange{domain_base + APLIC_IN_CLRIP, sizeof(uint32_t) * 32};
			in_clrip[ii] = new ArrayView<uint32_t>(*regs_in_clrip[ii]);
			(*regs_in_clrip[ii]).post_write_callback = std::bind(&APLIC::post_write_in_clrip, this, std::placeholders::_1);
			(*regs_in_clrip[ii]).pre_read_callback = std::bind(&APLIC::pre_read_in_clrip, this, std::placeholders::_1);

			regs_clripnum[ii] = new RegisterRange{domain_base + APLIC_CLRIPNUM, sizeof(uint32_t) * 1};
			clripnum[ii] = new IntegerView<uint32_t>(*regs_clripnum[ii]);
			(*regs_clripnum[ii]).post_write_callback = std::bind(&APLIC::post_write_clripnum, this, std::placeholders::_1);

			regs_setie[ii] = new RegisterRange{domain_base + APLIC_SETIE, sizeof(uint32_t) * 32};
			setie[ii] = new ArrayView<uint32_t>(*regs_setie[ii]);
			(*regs_setie[ii]).post_write_callback = std::bind(&APLIC::post_write_setie, this, std::placeholders::_1);
			(*regs_setie[ii]).pre_read_callback = std::bind(&APLIC::pre_read_setie, this, std::placeholders::_1);

			regs_setienum[ii] = new RegisterRange{domain_base + APLIC_SETIENUM, sizeof(uint32_t) * 1};
			setienum[ii] = new IntegerView<uint32_t>(*regs_setienum[ii]);
			(*regs_setienum[ii]).post_write_callback = std::bind(&APLIC::post_write_setienum, this, std::placeholders::_1);

			regs_clrie[ii] = new RegisterRange{domain_base + APLIC_CLRIE, sizeof(uint32_t) * 32};
			clrie[ii] = new ArrayView<uint32_t>(*regs_clrie[ii]);
			(*regs_clrie[ii]).post_write_callback = std::bind(&APLIC::post_write_clrie, this, std::placeholders::_1);

			regs_clrienum[ii] = new RegisterRange{domain_base + APLIC_CLRIENUM, sizeof(uint32_t) * 1};
			clrienum[ii] = new IntegerView<uint32_t>(*regs_clrienum[ii]);
			(*regs_clrienum[ii]).post_write_callback = std::bind(&APLIC::post_write_clrienum, this, std::placeholders::_1);

			regs_setipnum_le[ii] = new RegisterRange{domain_base + APLIC_SETIPNUMLE, sizeof(uint32_t) * 1};
			setipnum_le[ii] = new IntegerView<uint32_t>(*regs_setipnum_le[ii]);
			(*regs_setipnum_le[ii]).post_write_callback = std::bind(&APLIC::post_write_setipnum_le, this, std::placeholders::_1);

			regs_setipnum_be[ii] = new RegisterRange{domain_base + APLIC_SETIPNUMBE, sizeof(uint32_t) * 1};
			setipnum_be[ii] = new IntegerView<uint32_t>(*regs_setipnum_be[ii]);
			(*regs_setipnum_be[ii]).post_write_callback = std::bind(&APLIC::post_write_setipnum_be, this, std::placeholders::_1);

			regs_genmsi[ii] = new RegisterRange{domain_base + APLIC_GENMSI, sizeof(uint32_t) * 1};
			genmsi[ii] = new IntegerView<uint32_t>(*regs_genmsi[ii]);
			(*regs_genmsi[ii]).post_write_callback = std::bind(&APLIC::post_write_genmsi, this, std::placeholders::_1);

			regs_target[ii] = new RegisterRange{domain_base + APLIC_TARGET, sizeof(uint32_t) * 1023};
			target[ii] = new ArrayView<uint32_t>(*regs_target[ii]);
			(*regs_target[ii]).post_write_callback = std::bind(&APLIC::post_write_target, this, std::placeholders::_1);

			register_ranges.push_back(&(*regs_domaincfg[ii]));
			register_ranges.push_back(&(*regs_sourcecfg[ii]));
			register_ranges.push_back(&(*regs_mmsiaddrcfg[ii]));
			register_ranges.push_back(&(*regs_mmsiaddrcfgh[ii]));
			register_ranges.push_back(&(*regs_smsiaddrcfg[ii]));
			register_ranges.push_back(&(*regs_smsiaddrcfgh[ii]));
			register_ranges.push_back(&(*regs_setip[ii]));
			register_ranges.push_back(&(*regs_setipnum[ii]));
			register_ranges.push_back(&(*regs_in_clrip[ii]));
			register_ranges.push_back(&(*regs_clripnum[ii]));
			register_ranges.push_back(&(*regs_setie[ii]));
			register_ranges.push_back(&(*regs_setienum[ii]));
			register_ranges.push_back(&(*regs_clrie[ii]));
			register_ranges.push_back(&(*regs_clrienum[ii]));
			register_ranges.push_back(&(*regs_setipnum_le[ii]));
			register_ranges.push_back(&(*regs_setipnum_be[ii]));
			register_ranges.push_back(&(*regs_genmsi[ii]));
			register_ranges.push_back(&(*regs_target[ii]));
		}

		clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
		tsock.register_b_transport(this, &APLIC::transport);

		uint32_t domaincfg_reg;
		domaincfg_reg = 0x80 << APLIC_DOMAINCFG_RO80_BIT;
		domaincfg_reg |= BIT(APLIC_DOMAINCFG_DM_BIT); /* Only MSI delivery mode support */

		for (unsigned ii = 0; ii < NumberDomains; ii++) {
			*domaincfg[ii] = domaincfg_reg;

			for (unsigned jj = 1; jj <= NumberInterrupts; jj++) {
				(*sourcecfg[ii])[jj - 1] = 0;
			}

			for (int jj = 0; jj < 32; jj++) {
				(*setip[ii])[jj] = 0;
				(*in_clrip[ii])[jj] = 0;
				(*setie[ii])[jj] = 0;
				(*clrie[ii])[jj] = 0;

				ie_reg[ii][jj] = 0;
				ip_reg[ii][jj] = 0;
			}

			*setipnum[ii] = 0;
			*clripnum[ii] = 0;
			*setienum[ii] = 0;
			*clrienum[ii] = 0;
			*setipnum_le[ii] = 0;
			*setipnum_be[ii] = 0;
			*genmsi[ii] = 0;

			for (int jj = 1; jj <= 1023; jj++) {
				(*target[ii])[jj - 1] = 0;
			}

			if (ii == APLIC_M_DOMAIN) {
				uint32_t msi_addr;

				msi_addr = IMSIC_HART0_M_IFILE;
				*mmsiaddrcfg[ii] = msi_addr >> APLIC_PAGE_BITS;
				*mmsiaddrcfgh[ii] = 0;

				msi_addr = IMSIC_HART0_S_IFILE;
				*smsiaddrcfg[ii] = msi_addr >> APLIC_PAGE_BITS;
				*smsiaddrcfgh[ii] = 0;
			} else {
				*mmsiaddrcfg[ii] = 0;
				*mmsiaddrcfgh[ii] = 0;
				*smsiaddrcfg[ii] = 0;
				*smsiaddrcfgh[ii] = 0;
			}
		}

		SC_THREAD(run);
	}

	void post_write_domainconfig(RegisterRange::WriteInfo t)
	{
		uint32_t *domaincfg = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t tmp = *domaincfg;
		uint32_t domaincfg_reg = 0;

		domaincfg_reg &= ~BIT(APLIC_DOMAINCFG_BE_BIT); /* Always little endian */
		domaincfg_reg |= BIT(APLIC_DOMAINCFG_DM_BIT); /* Only MSI delivery mode support */
		domaincfg_reg |= tmp & (APLIC_DOMAINCFG_IE_MASK << APLIC_DOMAINCFG_IE_BIT);
		domaincfg_reg |= 0x80 << APLIC_DOMAINCFG_RO80_BIT;

		*domaincfg = domaincfg_reg;
	}

	void post_write_sourcecfg(RegisterRange::WriteInfo t)
	{
		uint32_t *sourcecfg = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t eiid = (t.addr >> 2) + 1;
		uint32_t sourcecfg_reg = *sourcecfg;
		uint32_t sm;

		sm = (sourcecfg_reg >> APLIC_SOURCECFG_SM_BIT) & APLIC_SOURCECFG_SM_MASK;
		assert(sm <= LEVEL0 && sm != RESERVED1 && sm != RESERVED2);

		assert(eiid > 0 && eiid < 1024);

		unsigned idx = eiid / 32;
		unsigned off = eiid % 32;

		if (sm == INACTIVE) {
			*sourcecfg = 0;
			(*target[APLIC_M_DOMAIN])[eiid - 1] = 0;
			ie_reg[APLIC_M_DOMAIN][idx] &= ~BIT(off);
			ip_reg[APLIC_M_DOMAIN][idx] &= ~BIT(off);
			return;
		}

		if (sm == DETACHED) {
			// ToDo: Add code here?
		}

		if (sourcecfg_reg & BIT(APLIC_SOURCECFG_D_BIT) ) {
			/* APLIC_SOURCECFG_CHILDIND_BIT specifies the interrupt domain to which this source is delegated */
			assert(((*sourcecfg >> APLIC_SOURCECFG_CHILDIND_BIT) & APLIC_SOURCECFG_CHILDIND_MASK) < NumberDomains );
		}

		/* ToDo: When source is not implemented => *sourcecfg = 0 */
	}

	void post_write_xmsiaddrcfg(RegisterRange::WriteInfo t)
	{
		uint32_t *addr = reinterpret_cast<uint32_t *>(t.var_addr);

		/* When a domain does not implement xmsiaddrcfg, xmsiaddrcfgh the eight bytes
			at their locations are simply read-only zeros */
		*addr = 0;
	}

	void post_write_setip(RegisterRange::WriteInfo t)
	{
		uint32_t *setip = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t idx = t.addr >> 2;
		uint32_t irq_id;
		uint32_t setip_reg = *setip;

		assert(idx < 32);

		for (int ii = 0; ii < 32; ii++) {
			if ((setip_reg & BIT(ii)) == 0 ) {
				continue;
			}
			irq_id = ii + 32 * idx;
			if (irq_id == 0) {
				setip_reg &= ~BIT(0);
				continue;
			}
			if (irq_id >= NumberInterrupts) {
				setip_reg &= BIT(ii) - 1;
				break;
			}
			if (source_is_inactive(APLIC_M_DOMAIN, irq_id)) {
				printf("Source inactive\n");
				setip_reg &= ~BIT(ii);
				continue;
			}
		}
		if (setip_reg) {
			ip_reg[APLIC_M_DOMAIN][idx] |= setip_reg;
			e_run.notify(clock_cycle); /* We have set a pending bit, if source is enabled we need to send MSI */
		}
	}

	bool pre_read_setip(RegisterRange::ReadInfo t)
	{
		uint32_t *setip = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t idx = t.addr >> 2;

		assert(idx < 32);

		*setip = ip_reg[APLIC_M_DOMAIN][idx]; /* Read of setip return pending bits of the sources */

		return true;
	}

	void post_write_setipnum(RegisterRange::WriteInfo t)
	{
		uint32_t *setipnum = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t irq_id = *setipnum;

		*setipnum = 0; /* A read of setipnum always returns zero */

		if (irq_id < 1 || irq_id >= NumberInterrupts)
			return;

		if (source_is_inactive(APLIC_M_DOMAIN, irq_id))
			return;

		unsigned idx = irq_id / 32;
		unsigned off = irq_id % 32;

		ip_reg[APLIC_M_DOMAIN][idx] |= BIT(off);
		(*setip[APLIC_M_DOMAIN])[idx] = ip_reg[APLIC_M_DOMAIN][idx]; /* Read of setip return pending bits of the sources */

		e_run.notify(clock_cycle); /* We have set a pending bit, if source is enabled we need to send MSI */
	}

	void post_write_in_clrip(RegisterRange::WriteInfo t)
	{
		uint32_t *in_clrip = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t idx = t.addr >> 2;
		uint32_t in_clrip_reg = *in_clrip;

		assert(idx < 32);

		ip_reg[APLIC_M_DOMAIN][idx] &= ~(in_clrip_reg);
	}

	bool pre_read_in_clrip(RegisterRange::ReadInfo t)
	{
		uint32_t *in_clrip = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t idx = t.addr >> 2;

		assert(idx < 32);

		/* A read of an in clrip register returns the rectified input
			values of the corresponding interrupt sources */
		*in_clrip = ip_reg[APLIC_M_DOMAIN][idx];

		return true;
	}

	void post_write_clripnum(RegisterRange::WriteInfo t)
	{
		uint32_t *clripnum = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t clripnum_reg = *clripnum;

		*clripnum = 0; /* A read of clripnum always returns zero */

		if (clripnum_reg < 1 || clripnum_reg >= NumberInterrupts)
			return;

		unsigned idx = clripnum_reg / 32;
		unsigned off = clripnum_reg % 32;

		ip_reg[APLIC_M_DOMAIN][idx] &= ~BIT(off);
		(*setip[APLIC_M_DOMAIN])[idx] = ip_reg[APLIC_M_DOMAIN][idx]; /* Read of setip return pending bits of the sources */
	}

	void post_write_setie(RegisterRange::WriteInfo t)
	{
		uint32_t *setie = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t idx = t.addr >> 2;
		uint32_t irq_id;
		uint32_t setie_reg = *setie;

		assert(idx < 32);

		/* Whenever interrupt source i is inactive in an interrupt domain,
		 * the corresponding interrupt-pending and interrupt-enable bits
		 * within the domain are read-only zeros, and register target[i] is also read-only zero
		 */
		for (int ii = 0; ii < 32; ii++) {
			if ((setie_reg & BIT(ii)) == 0 ) {
				continue;
			}
			irq_id = ii + 32 * idx;
			if (irq_id == 0) {
				setie_reg &= ~BIT(0);
				continue;
			}
			if (irq_id >= NumberInterrupts) {
				setie_reg &= BIT(ii) - 1;
				break;
			}
			if (source_is_inactive(APLIC_M_DOMAIN, irq_id)) { /* ToDo: Add APLIC_S_DOMAIN */
				setie_reg &= ~BIT(ii);
				continue;
			}
		}
		ie_reg[APLIC_M_DOMAIN][idx] |= setie_reg;

		if (ie_reg[APLIC_M_DOMAIN][idx])
			e_run.notify(clock_cycle);
	}

	bool pre_read_setie(RegisterRange::ReadInfo t)
	{
		uint32_t *setie = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t idx = t.addr >> 2;

		assert(idx < 32);

		*setie = ie_reg[APLIC_M_DOMAIN][idx]; /* Read of setie return enable bits of the sources */

		return true;
	}

	void post_write_setienum(RegisterRange::WriteInfo t)
	{
		uint32_t *setienum = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t irq_id = *setienum;

		*setienum = 0; /* A read of setienum always returns zero */

		if (irq_id < 1 || irq_id >= NumberInterrupts)
			return;

		if (source_is_inactive(APLIC_M_DOMAIN, irq_id))
			return;

		unsigned idx = irq_id / 32;
		unsigned off = irq_id % 32;

		ie_reg[APLIC_M_DOMAIN][idx] |= BIT(off);
		(*setie[APLIC_M_DOMAIN])[idx] = ie_reg[APLIC_M_DOMAIN][idx]; /* Read of setie return enable bits of the sources */

		e_run.notify(clock_cycle);
	}

	void post_write_clrie(RegisterRange::WriteInfo t)
	{
		uint32_t *clrie = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t idx = t.addr >> 2;
		uint32_t clrie_reg = *clrie;

		*clrie = 0; /* A read of a clrie register always returns zero */

		assert(idx < 32);

		ie_reg[APLIC_M_DOMAIN][idx] &= ~(clrie_reg);
		(*setie[APLIC_M_DOMAIN])[idx] = ie_reg[APLIC_M_DOMAIN][idx]; /* Read of setie return enable bits of the sources */
	}

	void post_write_clrienum(RegisterRange::WriteInfo t)
	{
		uint32_t *clrienum = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t clrienum_reg = *clrienum;

		*clrienum = 0; /* A read of clrienum always returns zero */

		if (clrienum_reg < 1 || clrienum_reg >= NumberInterrupts)
			return;

		if (source_is_inactive(APLIC_M_DOMAIN, clrienum_reg))
			return;

		unsigned idx = clrienum_reg / 32;
		unsigned off = clrienum_reg % 32;

		ie_reg[APLIC_M_DOMAIN][idx] &= ~BIT(off);
		(*setie[APLIC_M_DOMAIN])[idx] = ie_reg[APLIC_M_DOMAIN][idx]; /* Read of setie return enable bits of the sources */
	}

	void post_write_setipnum_le(RegisterRange::WriteInfo t)
	{
		post_write_setipnum(t); /* ToDO: need to complete */
	}

	void post_write_setipnum_be(RegisterRange::WriteInfo t)
	{
		uint32_t *setipnum_be = reinterpret_cast<uint32_t *>(t.var_addr);

		*setipnum_be = 0;
	}

	void post_write_genmsi(RegisterRange::WriteInfo t)
	{
		uint32_t *genmsi = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t hart_ind;
		uint32_t addr;
		uint32_t msi_data;

		*genmsi &= ~BIT(APLIC_GENMSI_BUSY_BIT); /* Always ready to send new msi */
		hart_ind = (*genmsi >> APLIC_GENMSI_HARTIND_BIT) & APLIC_GENMSI_HARTIND_MASK; /* Dest. hart */

		/* Address to send MSI */
		addr = get_m_target_address(hart_ind);

		/* MSI is sent even if domaincfg.IE = 0 */
		msi_data = (*genmsi >> APLIC_GENMSI_EIID_BIT) & APLIC_GENMSI_EIID_MASK; /* Data value for the MSI */

		sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
		tlm::tlm_generic_payload trans;
		trans.set_command(tlm::TLM_WRITE_COMMAND);
		trans.set_address(addr);
		trans.set_data_ptr(reinterpret_cast<unsigned char *>(&msi_data));
		trans.set_data_length(sizeof(uint32_t));
		isock->b_transport(trans, delay); /* Send MSI */

		/* All MSIs previously sent from this APLIC to the same hart must be visible at the
			hart’s IMSIC before the extempore MSI becomes visible at the hart’s IMSIC. */
		/* Extempore MSIs are not affected by the IE bit of the domain’s domaincfg register. An extempore
			MSI is sent even if domaincfg.IE = 0 */
	}

	void post_write_target(RegisterRange::WriteInfo t)
	{
		uint32_t *target = reinterpret_cast<uint32_t *>(t.var_addr);
		uint32_t idx = t.addr >> 2;
		uint32_t irq_id = idx + 1;

		if (source_is_inactive(APLIC_M_DOMAIN, irq_id)) {
			*target = 0; /* If interrupt source i is inactive in this domain,
								register target[i] is read-only zero */
			return;
		}

		*target |= BIT(APLIC_TARGETS_MSIDELIV_BIT); /* Only MMSI delivery support */
	}

	void gateway_trigger_interrupt(uint32_t irq_id)
	{
		if (irq_id < 1 || irq_id >= NumberInterrupts)
			return;

		if (source_is_inactive(APLIC_M_DOMAIN, irq_id))
			return;

		unsigned idx = irq_id / 32;
		unsigned off = irq_id % 32;

		ip_reg[APLIC_M_DOMAIN][idx] |= BIT(off);
		(*setip[APLIC_M_DOMAIN])[idx] = ip_reg[APLIC_M_DOMAIN][idx]; /* Read of setip return pending bits of the sources */

		e_run.notify(clock_cycle); /* We have set a pending bit, if source is enabled we need to send MSI */
	}

	bool is_pending_interrupt(uint32_t domain, uint32_t irq_id)
	{
		if (irq_id < 1 || irq_id >= NumberInterrupts)
			return false;

		unsigned idx = irq_id / 32;
		unsigned off = irq_id % 32;

		if ((ie_reg[domain][idx] & BIT(off)) == 0) {
			return false;
		}

		return ip_reg[domain][idx] & BIT(off);
	}

	void clear_pending_interrupt(int domain, uint32_t irq_id)
	{
		if (irq_id < 1 || irq_id >= NumberInterrupts)
			return;

		unsigned idx = irq_id / 32;
		unsigned off = irq_id % 32;

		ip_reg[domain][idx] &= ~BIT(off);
		(*setip[domain])[idx] = ip_reg[domain][idx]; /* Read of setip return pending bits of the sources */
	}

	bool source_is_inactive(int domain, uint32_t irq_id)
	{
		uint32_t val;

		if (irq_id < 1 || irq_id >= NumberInterrupts)
			return true;

		val = (*sourcecfg[domain])[irq_id - 1];
		if ((((val >> APLIC_SOURCECFG_SM_BIT) & APLIC_SOURCECFG_SM_MASK) < EDGE1/*DETACHED*/) ||
			((val >> APLIC_SOURCECFG_D_BIT) & APLIC_SOURCECFG_D_MASK)) {
			/* An interrupt source is inactive in the interrupt domain if either the source
				is delegated to a child domain (D = 1) or it is not delegated (D = 0) and SM is Inactive */
			return true;
		}
		return false;
	}

	unsigned hart_get_next_pending_interrupt(int domain)
	{
		uint32_t irq_id;

		if ((((*domaincfg[domain]) >> APLIC_DOMAINCFG_DM_BIT) & APLIC_DOMAINCFG_DM_MASK) == 0) {
			return 0; /* Direct delivery mode is not supported */
		}

		if ((((*domaincfg[domain]) >> APLIC_DOMAINCFG_IE_BIT) & APLIC_DOMAINCFG_IE_MASK) == 0) {
			return 0; /* All interrupts are disabled */
		}

		for (irq_id = 1; irq_id < NumberInterrupts; irq_id++) { /* ToDo: Rewrite this algorithm!! */
			if (source_is_inactive(domain, irq_id)) {
				continue;
			}

			if (is_pending_interrupt(domain, irq_id))
				break;
		}

		if (irq_id == NumberInterrupts)
			return 0;

		return irq_id;
	}

	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay)
	{
		delay += 4 * clock_cycle;
		vp::mm::route("APLIC", register_ranges, trans, delay);
	}

	uint32_t get_m_target_address(uint32_t hart_ind /* From target[i] */)
	{
		uint64_t ppn, hippn;
		uint64_t msi_addr;
		uint32_t mmsiaddr, mmsiaddrh;
		uint32_t g, h;
		uint32_t lhxw, hhxw, hhxs, lhxs;

		assert(hart_ind < NumberCores);

		mmsiaddr = *mmsiaddrcfg[APLIC_M_DOMAIN];
		mmsiaddrh = *mmsiaddrcfgh[APLIC_M_DOMAIN];
		lhxw = (mmsiaddrh >> APLIC_MMSIADDRCFGH_LHXW_BIT) & APLIC_MMSIADDRCFGH_LHXW_MASK;
		hhxw = (mmsiaddrh >> APLIC_MMSIADDRCFGH_HHXW_BIT) & APLIC_MMSIADDRCFGH_HHXW_MASK;
		lhxs = (mmsiaddrh >> APLIC_MMSIADDRCFGH_LHXS_BIT) & APLIC_MMSIADDRCFGH_LHXS_MASK;
		hhxs = (mmsiaddrh >> APLIC_MMSIADDRCFGH_HHXS_BIT) & APLIC_MMSIADDRCFGH_HHXS_MASK;
		hippn = (mmsiaddrh >> APLIC_MMSIADDRCFGH_HIPPN_BIT) & APLIC_MMSIADDRCFGH_HIPPN_MASK;
		g = (hart_ind >> lhxw) & ((1 << hhxw) - 1); /* The number of a hart group */
		h = hart_ind & ((1 << lhxw) - 1); /* The number of the target hart within that group */
		ppn = (hippn << 32) | mmsiaddr;
		msi_addr = (  ppn | (g << (hhxs + 12)) | (h << lhxs)  ) << 12;

		return (uint32_t)msi_addr;
	}

	uint32_t get_s_target_address(uint32_t machine_hart_ind, uint32_t guest_ind/* From target[i] */)
	{
		uint64_t ppn, hippn;
		uint64_t msi_addr;
		uint32_t mmsiaddr, mmsiaddrh;
		uint32_t smsiaddr, smsiaddrh;
		uint32_t g, h;
		uint32_t lhxw, hhxw, hhxs, lhxs;
		uint32_t offs;

		assert(machine_hart_ind < NumberCores);

		mmsiaddr = *mmsiaddrcfg[APLIC_M_DOMAIN];
		mmsiaddrh = *mmsiaddrcfgh[APLIC_M_DOMAIN];
		smsiaddr = *smsiaddrcfg[APLIC_M_DOMAIN];
		smsiaddrh = *smsiaddrcfgh[APLIC_M_DOMAIN];
		lhxw = (mmsiaddrh >> APLIC_MMSIADDRCFGH_LHXW_BIT) & APLIC_MMSIADDRCFGH_LHXW_MASK;
		hhxw = (mmsiaddrh >> APLIC_MMSIADDRCFGH_HHXW_BIT) & APLIC_MMSIADDRCFGH_HHXW_MASK;
		lhxs = (smsiaddrh >> APLIC_SMSIADDRCFGH_LHXS_BIT) & APLIC_SMSIADDRCFGH_LHXS_MASK;
		hhxs = (mmsiaddrh >> APLIC_MMSIADDRCFGH_HHXS_BIT) & APLIC_MMSIADDRCFGH_HHXS_MASK;
		hippn = (smsiaddrh >> APLIC_SMSIADDRCFGH_HIPPN_BIT) & APLIC_SMSIADDRCFGH_HIPPN_MASK;
		g = (machine_hart_ind >> lhxw) & ((1 << hhxw) - 1); /* The number of a hart group */
		h = machine_hart_ind & ((1 << lhxw) - 1); /* The number of the target hart within that group */
		ppn = (hippn << 32ULL) | smsiaddr;
		msi_addr = (  ppn | (g << (hhxs + 12)) | (h << lhxs) | guest_ind ) << 12;

		return (uint32_t)msi_addr;
	}

	void msi_clr_pending(uint32_t domain, uint32_t irq_id)
	{
		uint32_t cfg;
		uint32_t sm;

		if (irq_id < 1 || irq_id >= NumberInterrupts)
			return;

		cfg = (*sourcecfg[domain])[irq_id - 1];
		sm = (cfg >> APLIC_SOURCECFG_SM_BIT) & APLIC_SOURCECFG_SM_MASK;

		if((sm != EDGE0) && (sm != EDGE1)) /* Clear only edge triggered interrupt */
			return;

		unsigned idx = irq_id / 32;
		unsigned off = irq_id % 32;

		ip_reg[domain][idx] &= ~BIT(off);
		(*setip[domain])[idx] = ip_reg[domain][idx]; /* Read of setip return pending bits of the sources */
	}

	void msi_write(uint32_t domain, uint32_t irq_id)
	{
		uint32_t tgt;
		uint32_t addr;
		uint32_t hart_ind;
		uint32_t msi_data;

		if (irq_id < 1 || irq_id >= NumberInterrupts)
			return;

		if (source_is_inactive(domain, irq_id)) {
			return;
		}

		tgt = (*target[domain])[irq_id - 1];
		hart_ind = (tgt >> APLIC_TARGETS_HARTIND_BIT) & APLIC_TARGETS_HARTIND_MASK;
		addr = get_m_target_address(hart_ind);

		/* MSI is sent even if domaincfg.IE = 0 */
		msi_data = (tgt >> APLIC_TARGETS_EIID_BIT) & APLIC_TARGETS_EIID_MASK; /* MSI data: zero extended EIID field */

		sc_core::sc_time delay = sc_core::SC_ZERO_TIME;
		tlm::tlm_generic_payload trans;
		trans.set_command(tlm::TLM_WRITE_COMMAND);
		trans.set_address(addr);
		trans.set_data_ptr(reinterpret_cast<unsigned char *>(&msi_data));
		trans.set_data_length(sizeof(uint32_t));
		isock->b_transport(trans, delay); /* Send MSI */

		/* Clear pending bit, if edge level triggered */
		msi_clr_pending(domain, irq_id);
	}

	void run()
	{
		unsigned int_id;

		while (true) {
			sc_core::wait(e_run);

			int_id = hart_get_next_pending_interrupt(APLIC_M_DOMAIN);
			if (int_id > 0) {
				msi_write(APLIC_M_DOMAIN, int_id);
			}
		}
	}
};
