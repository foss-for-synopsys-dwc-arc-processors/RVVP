#pragma once

#include <tlm_utils/simple_target_socket.h>
#include <systemc>

#include "imsic-if.h"
#include "util/memory_map.h"

struct ImsicMem : public sc_core::sc_module {
	tlm_utils::simple_target_socket<ImsicMem> tsock;

	unsigned page_size = 4096;

	// 64 x 32bit regs
	unsigned max_imsic_iid = 32 * 64 - 1;

	// page size * imsic per core number
	RegisterRange regs_imsic_mmio{0x0, page_size * (1 + 1 + iss_config::MAX_GUEST)};
	ArrayView<uint32_t> imsic_mmio{regs_imsic_mmio};

	std::vector<RegisterRange *> register_ranges{&regs_imsic_mmio};

	imsic_mem_target * target_hart;

	SC_HAS_PROCESS(ImsicMem);

	ImsicMem(sc_core::sc_module_name, uint32_t hart_id, imsic_mem_target *target_hart) : target_hart(target_hart) {
		tsock.register_b_transport(this, &ImsicMem::transport);

		regs_imsic_mmio.alignment = 4;
		regs_imsic_mmio.post_write_callback = std::bind(&ImsicMem::post_write_imsic_mmio, this, std::placeholders::_1);
		regs_imsic_mmio.pre_read_callback = std::bind(&ImsicMem::pre_read_imsic_mmio, this, std::placeholders::_1);
	}

	bool pre_read_imsic_mmio(RegisterRange::ReadInfo t) {
		assert(t.addr % 4 == 0); // TODO: replace with a bus error

		// A read of seteipnum le or seteipnum be returns zero in all cases.
		// All other bytes in an interrupt file’s 4-KiB memory region are reserved and must
		// be implemented as read-only zeros.
		imsic_mmio[t.addr / 4] = 0;

		// printf("[vp::imsic] got read: addr %lx\n", t.addr);

		return true;
	}

	void post_write_imsic_mmio(RegisterRange::WriteInfo t) {
		assert(t.addr % 4 == 0);

		if (t.addr % page_size == 0) {
			unsigned imsic_idx = t.addr / page_size;
			unsigned mem_idx = t.addr / 4;

			assert(imsic_idx < (1 + 1 + iss_config::MAX_GUEST));

			uint32_t value = imsic_mmio[mem_idx] & max_imsic_iid;

			// printf("[vp::imsic] got write: addr %lx, idx %u, val %u\n", t.addr, imsic_idx, value);

			if (imsic_idx == 0)
				target_hart->route_imsic_write(MachineMode, 0, value);
			else if (imsic_idx == 1)
				target_hart->route_imsic_write(SupervisorMode, 0, value);
			else
				target_hart->route_imsic_write(VirtualSupervisorMode, imsic_idx - 2, value);
		} else {
			printf("[vp::imsic] write ignored, addr: %lx\n", t.addr);
		}
	}

	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
		// delay += 2 * clock_cycle;

		vp::mm::route("ImsicMem", register_ranges, trans, delay);
	}
};
