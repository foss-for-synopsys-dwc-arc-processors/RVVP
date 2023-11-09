#pragma once

#include "core/common/dmi.h"
#include "iss.h"
#include "core/common/protected_access.h"
#include "mmu.h"
#include "spmp.h"
#include "smpu.h"

namespace rv32 {

/* For optimization, use DMI to fetch instructions */
struct InstrMemoryProxy : public instr_memory_if {
	MemoryDMI dmi;

	tlm_utils::tlm_quantumkeeper &quantum_keeper;
	sc_core::sc_time clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
	sc_core::sc_time access_delay = clock_cycle * 2;

	InstrMemoryProxy(const MemoryDMI &dmi, ISS &owner) : dmi(dmi), quantum_keeper(owner.quantum_keeper) {}

	virtual uint32_t load_instr(uint64_t pc) override {
		quantum_keeper.inc(access_delay);
		return dmi.load<uint32_t>(pc);
	}
};

struct CombinedMemoryInterface : public sc_core::sc_module,
                                 public instr_memory_if,
                                 public data_memory_if,
                                 public mmu_memory_if,
                                 public spmp_memory_if,
                                 public smpu_memory_if {
	ISS &iss;
	std::shared_ptr<bus_lock_if> bus_lock;
	uint64_t lr_addr = 0;

	tlm_utils::simple_initiator_socket<CombinedMemoryInterface> isock;
	tlm_utils::tlm_quantumkeeper &quantum_keeper;

	// optionally add DMI ranges for optimization
	sc_core::sc_time clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);
	sc_core::sc_time dmi_access_delay = clock_cycle * 4;
	std::vector<MemoryDMI> dmi_ranges;

    MMU *mmu;
    SPMP *spmp;
    SMPU *smpu;

	CombinedMemoryInterface(sc_core::sc_module_name, ISS &owner, MMU *mmu = nullptr,
	      SPMP *spmp = nullptr, SMPU *smpu = nullptr)
	    : iss(owner), quantum_keeper(iss.quantum_keeper), mmu(mmu), spmp(spmp), smpu(smpu) {
	}

	inline bool phya_spmp_check(PrivilegeLevel mode, uint64_t paddr, uint32_t sz,
			MemoryAccessType type) override {
		if (spmp == nullptr)
			return false;
		return spmp->do_phy_address_check(mode, paddr, sz, type);
	}

	inline bool phya_smpu_check(PrivilegeLevel mode, uint64_t *pa_va_ddr, uint32_t sz,
		MemoryAccessType type, SmpuLevel level, bool is_hlvx_access = false) override {
		if (smpu == nullptr)
			return false;
		return smpu->do_phy_address_check(mode, pa_va_ddr, sz, type, level, is_hlvx_access);
	}

    uint64_t v2p(uint64_t vaddr, MemoryAccessType type) override {
	    if (mmu == nullptr)
	        return vaddr;
        return mmu->translate_virtual_to_physical_addr(vaddr, type);
    }

	inline void _do_transaction(tlm::tlm_command cmd, uint64_t addr, uint8_t *data, unsigned num_bytes) {
		tlm::tlm_generic_payload trans;
		trans.set_command(cmd);
		trans.set_address(addr);
		trans.set_data_ptr(data);
		trans.set_data_length(num_bytes);
		trans.set_response_status(tlm::TLM_OK_RESPONSE);

		sc_core::sc_time local_delay = quantum_keeper.get_local_time();

		isock->b_transport(trans, local_delay);

		assert(local_delay >= quantum_keeper.get_local_time());
		quantum_keeper.set(local_delay);

		if (trans.is_response_error()) {
			if (iss.trace || iss.sys)	// if iss has syscall interface, it likely has no traphandler for this
				std::cout << "WARNING: core memory transaction failed for address 0x" << std::hex << addr << std::dec << std::endl;
			if (cmd == tlm::TLM_READ_COMMAND)
				raise_trap(EXC_LOAD_PAGE_FAULT, addr);
			else if (cmd == tlm::TLM_WRITE_COMMAND)
				raise_trap(EXC_STORE_AMO_PAGE_FAULT, addr);
			else
				throw std::runtime_error("TLM command must be read or write");
		}
	}

	template <typename T>
	inline T _raw_load_data(uint64_t addr) {
		// NOTE: a DMI load will not context switch (SystemC) and not modify the memory, hence should be able to
		// postpone the lock after the dmi access
		bus_lock->wait_for_access_rights(iss.get_hart_id());

		for (auto &e : dmi_ranges) {
			if (e.contains(addr)) {
				quantum_keeper.inc(dmi_access_delay);
				return e.load<T>(addr);
			}
		}

		T ans;
		_do_transaction(tlm::TLM_READ_COMMAND, addr, (uint8_t *)&ans, sizeof(T));
		return ans;
	}

	template <typename T>
	inline void _raw_store_data(uint64_t addr, T value) {
		bus_lock->wait_for_access_rights(iss.get_hart_id());

		bool done = false;
		for (auto &e : dmi_ranges) {
			if (e.contains(addr)) {
				quantum_keeper.inc(dmi_access_delay);
				e.store(addr, value);
				done = true;
			}
		}

		if (!done)
			_do_transaction(tlm::TLM_WRITE_COMMAND, addr, (uint8_t *)&value, sizeof(T));
		atomic_unlock();
	}

	inline PrivilegeLevel get_mem_mode(MemoryAccessType type, PrivilegeLevel privilege_override) {
		auto mode = iss.prv;
		if (type != FETCH && iss.csrs.mstatus.fields.mprv)
			mode = iss.csrs.mstatus.fields.mpp;
		if (privilege_override != NoneMode)
			mode = privilege_override;

		return mode;
	}

	inline bool _phya_smpu_check(PrivilegeLevel mode, uint64_t *addr,
		uint32_t sz, MemoryAccessType type, bool is_hlvx_access = false) {

		bool smpu_done = phya_smpu_check(mode, addr, sz, type, SMPU_LEVEL_1, is_hlvx_access);
		smpu_done &= phya_smpu_check(mode, addr, sz, type, SMPU_LEVEL_2, is_hlvx_access);

		return smpu_done;
	}

	template <typename T>
	inline T _load_data(uint64_t addr, PrivilegeLevel privilege_override = NoneMode, bool is_hlvx_access = false) {
		auto mode = get_mem_mode(LOAD, privilege_override);

		if (iss.use_smpu) { // SMPU
			if (_phya_smpu_check(mode, &addr, sizeof(T), LOAD, is_hlvx_access))
				return _raw_load_data<T>(addr);
		} else if (iss.use_spmp) { // SPMP
			if (phya_spmp_check(mode, addr, sizeof(T), LOAD))
				return _raw_load_data<T>(addr);
		}
		/* satp.mode != Bare, then paged Virtual Memory only */
		return _raw_load_data<T>(v2p(addr, LOAD));
	}

	template <typename T>
	inline void _store_data(uint64_t addr, T value, PrivilegeLevel privilege_override = NoneMode) {
		auto mode = get_mem_mode(STORE, privilege_override);

		if (iss.use_smpu) { // SMPU
			if (_phya_smpu_check(mode, &addr, sizeof(T), STORE)) {
				_raw_store_data<T>(addr, value);
				return;
			}
		} else if (iss.use_spmp) { // SPMP
			if (phya_spmp_check(mode, addr, sizeof(T), STORE)) {
				_raw_store_data<T>(addr, value);
				return;
			}
		}
		_raw_store_data(v2p(addr, STORE), value);
	}

    uint64_t mmu_load_pte64(uint64_t addr) override {
        return _raw_load_data<uint64_t>(addr);
    }
    uint64_t mmu_load_pte32(uint64_t addr) override {
        return _raw_load_data<uint32_t>(addr);
    }
    void mmu_store_pte32(uint64_t addr, uint32_t value) override {
        _raw_store_data(addr, value);
    }

    void flush_tlb() override {
        mmu->flush_tlb();
    }

    void clear_spmp_cache() override {
        spmp->clear_spmp_cache();
    }

	uint32_t load_instr(uint64_t addr) override {
		auto mode = get_mem_mode(FETCH, NoneMode);

		if (iss.use_smpu) { // SMPU
			if (_phya_smpu_check(mode, &addr, sizeof(uint32_t), FETCH))
				return _raw_load_data<uint32_t>(addr);
		} else if (iss.use_spmp) { // SPMP
			if (phya_spmp_check(mode, addr, sizeof(uint32_t), FETCH))
				return _raw_load_data<uint32_t>(addr);
		}
		return _raw_load_data<uint32_t>(v2p(addr, FETCH));
	}

    int64_t load_double(uint64_t addr) override {
        return _load_data<int64_t>(addr);
    }
	int32_t load_half(uint64_t addr, PrivilegeLevel privilege_override = NoneMode) override {
		return _load_data<int16_t>(addr, privilege_override);
	}
	int32_t load_byte(uint64_t addr, PrivilegeLevel privilege_override = NoneMode) override {
		return _load_data<int8_t>(addr, privilege_override);
	}
	uint32_t load_ubyte(uint64_t addr, PrivilegeLevel privilege_override = NoneMode) override {
		return _load_data<uint8_t>(addr, privilege_override);
	}
	uint32_t load_uhalf(uint64_t addr, PrivilegeLevel privilege_override = NoneMode,
			bool is_hlvx_access = false) override {
		return _load_data<uint16_t>(addr, privilege_override, is_hlvx_access);
	}
	int32_t load_word(uint64_t addr, PrivilegeLevel privilege_override = NoneMode,
			bool is_hlvx_access = false) override {
		return _load_data<int32_t>(addr, privilege_override, is_hlvx_access);
	}

    void store_double(uint64_t addr, uint64_t value) override {
        _store_data(addr, value);
    }
	void store_word(uint64_t addr, uint32_t value, PrivilegeLevel privilege_override = NoneMode) override {
		_store_data(addr, value, privilege_override);
	}
	void store_half(uint64_t addr, uint16_t value, PrivilegeLevel privilege_override = NoneMode) override {
		_store_data(addr, value, privilege_override);
	}
	void store_byte(uint64_t addr, uint8_t value, PrivilegeLevel privilege_override = NoneMode) override {
		_store_data(addr, value, privilege_override);
	}

	virtual int32_t atomic_load_word(uint64_t addr) override {
		bus_lock->lock(iss.get_hart_id());
		return load_word(addr);
	}
	virtual void atomic_store_word(uint64_t addr, uint32_t value) override {
		assert(bus_lock->is_locked(iss.get_hart_id()));
		store_word(addr, value);
	}
	virtual int32_t atomic_load_reserved_word(uint64_t addr) override {
		bus_lock->lock(iss.get_hart_id());
		lr_addr = addr;
		return load_word(addr);
	}
	virtual bool atomic_store_conditional_word(uint64_t addr, uint32_t value) override {
		/* According to the RISC-V ISA, an implementation can fail each LR/SC sequence that does not satisfy the forward
		 * progress semantic.
		 * The lock is established by the LR instruction and the lock is kept while forward progress is maintained. */
		if (bus_lock->is_locked(iss.get_hart_id())) {
			if (addr == lr_addr) {
				store_word(addr, value);
				return true;
			}
			atomic_unlock();
		}
		return false;
	}
	virtual void atomic_unlock() override {
		bus_lock->unlock(iss.get_hart_id());
	}
};

}  // namespace rv32
