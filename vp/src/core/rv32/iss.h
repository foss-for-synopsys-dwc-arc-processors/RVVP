#pragma once

#include "core/common/bus_lock_if.h"
#include "core/common/clint_if.h"
#include "core/common/instr.h"
#include "core/common/irq_if.h"
#include "core/common/trap.h"
#include "core/common/debug.h"
#include "csr.h"
#include "fp.h"
#include "mem_if.h"
#include "syscall_if.h"
#include "util/common.h"
#include "csr-names.h"
#include "irq-helpers.h"
#include "irq-prio.h"
#include "trap-codes.h"

#include "imsic-mem.h"
#include "imsic-if.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <tuple>

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/tlm_quantumkeeper.h>
#include <systemc>

namespace rv32 {

struct RegFile {
	static constexpr unsigned NUM_REGS = 32;

	int32_t regs[NUM_REGS];

	RegFile();

	RegFile(const RegFile &other);

	void write(uint32_t index, int32_t value);

	int32_t read(uint32_t index);

	uint32_t shamt(uint32_t index);

	int32_t &operator[](const uint32_t idx);

	void show();

	enum e : uint16_t {
		x0 = 0,
		x1,
		x2,
		x3,
		x4,
		x5,
		x6,
		x7,
		x8,
		x9,
		x10,
		x11,
		x12,
		x13,
		x14,
		x15,
		x16,
		x17,
		x18,
		x19,
		x20,
		x21,
		x22,
		x23,
		x24,
		x25,
		x26,
		x27,
		x28,
		x29,
		x30,
		x31,

		zero = x0,
		ra = x1,
		sp = x2,
		gp = x3,
		tp = x4,
		t0 = x5,
		t1 = x6,
		t2 = x7,
		s0 = x8,
		fp = x8,
		s1 = x9,
		a0 = x10,
		a1 = x11,
		a2 = x12,
		a3 = x13,
		a4 = x14,
		a5 = x15,
		a6 = x16,
		a7 = x17,
		s2 = x18,
		s3 = x19,
		s4 = x20,
		s5 = x21,
		s6 = x22,
		s7 = x23,
		s8 = x24,
		s9 = x25,
		s10 = x26,
		s11 = x27,
		t3 = x28,
		t4 = x29,
		t5 = x30,
		t6 = x31,
	};
};

// NOTE: on this branch, currently the *simple-timing* model is still directly
// integrated in the ISS. Merge the *timedb* branch to use the timing_if.
struct ISS;

struct timing_if {
	virtual ~timing_if() {}

	virtual void update_timing(Instruction instr, Opcode::Mapping op, ISS &iss) = 0;
};

/* Buffer to be used between the ISS and instruction memory interface to cache compressed instructions.
 * In case the ISS does not support compressed instructions, then this buffer is not necessary and the ISS
 * can use the memory interface directly. */
struct InstructionBuffer {
	instr_memory_if *instr_mem = nullptr;
	uint32_t last_fetch_addr = 0;
	uint32_t buffer = 0;

	uint32_t load_instr(uint64_t addr) {
		if (addr == (last_fetch_addr + 2))
			return (buffer >> 16);

		last_fetch_addr = addr;
		buffer = instr_mem->load_instr(addr);
		return buffer;
	}
};

struct PendingInterrupts {
	uint64_t m_pending;
	uint64_t s_hs_pending;
	uint64_t vs_pending;
};

struct PendingIvtAccess {
	bool pending = 0;
	uint32_t entry_address = 0;
};

struct ISS : public external_interrupt_target, public clint_interrupt_target, public iss_syscall_if, public debug_target_if, public imsic_mem_target {
	clint_if *clint = nullptr;
	instr_memory_if *instr_mem = nullptr;
	data_memory_if *mem = nullptr;
	syscall_emulator_if *sys = nullptr;  // optional, if provided, the iss will intercept and handle syscalls directly
	RegFile regs;
	FpRegs fp_regs;
	uint32_t pc = 0;
	uint32_t last_pc = 0;
	PendingIvtAccess ivt_access;
	bool trace = false;
	bool shall_exit = false;
	bool ignore_wfi = false;
	bool error_on_zero_traphandler = false;
	csr_table csrs;
	icsr_ms_table icsrs_m = icsr_ms_table(MachineMode);
	icsr_ms_table icsrs_s = icsr_ms_table(SupervisorMode);
	icsr_vs_table icsrs_vs;
	PrivilegeLevel prv = MachineMode;
	int64_t lr_sc_counter = 0;
	uint64_t total_num_instr = 0;
	csr_name_mapping csr_names;
	icsr_name_mapping icsr_names;
	bool use_spmp = false;
	bool use_smpu = false;

	ImsicMem imsic;

	// last decoded and executed instruction and opcode
	Instruction instr;
	Opcode::Mapping op;

	CoreExecStatus status = CoreExecStatus::Runnable;
	std::unordered_set<uint32_t> breakpoints;
	bool debug_mode = false;

	sc_core::sc_event wfi_event;

	std::string systemc_name;
	tlm_utils::tlm_quantumkeeper quantum_keeper;
	sc_core::sc_time cycle_time;
	sc_core::sc_time cycle_counter;  // use a separate cycle counter, since cycle count can be inhibited
	std::array<sc_core::sc_time, Opcode::NUMBER_OF_INSTRUCTIONS> instr_cycles;

	static constexpr int32_t REG_MIN = INT32_MIN;
	static constexpr unsigned xlen = 32;

	ISS(uint32_t hart_id, bool use_E_base_isa = false);

	void hs_inst_check_access(void);
	PrivilegeLevel hs_inst_lvsv_mode(void);

	void exec_step();
	void set_pending_ivt(uint32_t address);
	void process_pending_ivt(void);

	uint64_t _compute_and_get_current_cycles();

	void init(instr_memory_if *instr_mem, data_memory_if *data_mem, clint_if *clint, uint32_t entrypoint, uint32_t sp);

	void trigger_external_interrupt(PrivilegeLevel level) override;
	void clear_external_interrupt(PrivilegeLevel level) override;
	void trigger_timer_interrupt(bool status, PrivilegeLevel timer) override;
	void trigger_software_interrupt(bool status, PrivilegeLevel sw_irq_type) override;

	uint64_t get_xtimecmp_level_csr(PrivilegeLevel level) override;
	bool is_timer_compare_level_exists(PrivilegeLevel level) override;

	void iprio_icsr_access_adjust(void);

	void deliver_clint_changes_to_imsics(PendingInterrupts old_pendings_0);
	void deliver_clint_changes_to_imsics(PendingInterrupts old_pendings_0, uint32_t iid);
	void cascade_pendings_to_imsics(PendingInterrupts pendings_1);
	void deliver_pending_to_imsic(PendingInterrupts prev_pendings, PendingInterrupts new_pendings, uint32_t iid);
	void clint_hw_irq_route(uint32_t iid, bool set);
	void route_imsic_write(PrivilegeLevel target_imsic, unsigned guest_index, uint32_t value) override;

	void on_guest_switch(void);
	void on_xenvcfgh_write(void);
	void on_xtvec_write(PrivilegeLevel level);
	void on_clint_csr_write(uint32_t addr, uint32_t value);
	void notify_irq_taken(PrivilegeLevel target_mode);

	void update_interrupt_mode(PrivilegeLevel level);

	void relax_xtopi(void);
	csr_mcause & get_xcause(PrivilegeLevel level);
	csr_mtvec & get_xtvec(PrivilegeLevel level);
	csr_topei & get_topei(PrivilegeLevel level);
	void set_topei(PrivilegeLevel level, uint32_t eiid);
	uint32_t get_vstopi_ipriom_adjusted(void);

	void claim_topei_interrupt_on_xtopei(PrivilegeLevel target_level, uint32_t value, bool access_write_only);
	void claim_topei_interrupt_internal(PrivilegeLevel target_level);
	void claim_topei_interrupt(PrivilegeLevel target_level, bool mark_irq_handled);

	void imsic_update_eip_bit(icsr_32 *eip, uint32_t value, bool set_bit);

	uint8_t get_iprio(PrivilegeLevel level, uint32_t iid);
	struct irq_cprio get_external_cprio(PrivilegeLevel level);
	struct irq_cprio get_external_cprio_generic(PrivilegeLevel level);
	struct irq_cprio get_external_cprio_256(PrivilegeLevel level);
	struct irq_cprio get_local_cprio(PrivilegeLevel level, uint32_t iid);

	void vstimecmp_access_check(void);
	void stimecmp_access_check(void);
	void vs_clint_vti_access_check(void);

	void vs_csr_icsrs_access_exception(void);
	void vstopei_access_check(void);
	void vs_icsrs_access_check(unsigned icsr_addr);
	void s_sei_injection_access_check(void);
	void s_icsr_imsic_access_check(unsigned icsr_addr);
	void sanitize_vs_external_pend(PendingInterrupts &irqs_pend);

	std::tuple<bool, uint32_t> compute_imsic_pending(icsr_32 *eip, icsr_32 *eie, uint32_t size, icsr_eithreshold &eithreshold, icsr_eidelivery &eidelivery, bool nested_vectored);
	void compute_imsic_pending_interrupts_m(void);
	void compute_imsic_pending_interrupts_s(void);
	void compute_imsic_pending_interrupts_vs(void);

	struct irq_cprio major_irq_to_prio(PrivilegeLevel target_level, uint32_t iid);
	std::tuple<uint32_t, uint8_t> major_irq_prepare_iid_prio(PrivilegeLevel level, uint64_t levels_pending);

	bool is_irq_globaly_enable_per_level(PrivilegeLevel target_mode);
	bool is_irq_mode_snps_nested_vectored(PrivilegeLevel target_mode);

	void recalc_sgeip(void);
	void recalc_xtopi(PendingInterrupts &irqs_pend);
	PrivilegeLevel compute_pending_interrupt(PendingInterrupts &irqs_pend);
	struct PendingInterrupts process_clint_pending_irq_bits_per_level(PendingInterrupts & pendings);
	struct PendingInterrupts compute_clint_pending_irq_bits_per_level(void);
	std::tuple<PrivilegeLevel, bool> prepare_interrupt(void);

	void sys_exit() override;
	unsigned get_syscall_register_index() override;
	uint64_t read_register(unsigned idx) override;
	void write_register(unsigned idx, uint64_t value) override;

	std::vector<uint64_t> get_registers(void) override;

	Architecture get_architecture(void) override {
		return RV32;
	}

	uint64_t get_progam_counter(void) override;
	void enable_debug(void) override;
	CoreExecStatus get_status(void) override;
	void set_status(CoreExecStatus) override;
	void block_on_wfi(bool) override;

	void insert_breakpoint(uint64_t) override;
	void remove_breakpoint(uint64_t) override;

	uint64_t get_hart_id() override;


	void release_lr_sc_reservation() {
		lr_sc_counter = 0;
		mem->atomic_unlock();
	}

	void fp_prepare_instr();
	void fp_finish_instr();
	void fp_set_dirty();
	void fp_update_exception_flags();
	void fp_setup_rm();
	void fp_require_not_off();

	uint32_t get_csr_value(uint32_t addr);
	void set_csr_value(uint32_t addr, uint32_t value, bool read_accessed);

	bool is_invalid_csr_access(uint32_t csr_addr, bool is_write);
	void validate_csr_counter_read_access_rights(uint32_t addr);

	uint32_t csr_address_virt_transform(uint32_t addr);

	unsigned pc_alignment_mask() {
		if (csrs.misa.has_C_extension())
			return ~0x1;
		else
			return ~0x3;
	}

	inline void trap_check_pc_alignment() {
		assert(!(pc & 0x1) && "not possible due to immediate formats and jump execution");

		if (unlikely((pc & 0x3) && (!csrs.misa.has_C_extension()))) {
			// NOTE: misaligned instruction address not possible on machines supporting compressed instructions
			raise_trap(EXC_INSTR_ADDR_MISALIGNED, pc);
		}
	}

	template <unsigned Alignment, bool isLoad>
	inline void trap_check_addr_alignment(uint32_t addr) {
		if (unlikely(addr % Alignment)) {
			raise_trap(isLoad ? EXC_LOAD_ADDR_MISALIGNED : EXC_STORE_AMO_ADDR_MISALIGNED, addr);
		}
	}

	inline void execute_amo(Instruction &instr, std::function<int32_t(int32_t, int32_t)> operation) {
		uint32_t addr = regs[instr.rs1()];
		trap_check_addr_alignment<4, false>(addr);
		uint32_t data;
		try {
			data = mem->atomic_load_word(addr);
		} catch (SimulationTrap &e) {
			if (e.reason == EXC_LOAD_ACCESS_FAULT)
				e.reason = EXC_STORE_AMO_ACCESS_FAULT;
			throw e;
		}
		uint32_t val = operation(data, regs[instr.rs2()]);
		mem->atomic_store_word(addr, val);
		regs[instr.rd()] = data;
	}

	inline bool m_mode() {
		return prv == MachineMode;
	}

	inline bool s_mode() {
		return prv == SupervisorMode;
	}

	inline bool vs_mode() {
		return prv == VirtualSupervisorMode;
	}

	inline bool u_mode() {
		return prv == UserMode;
	}

	uint32_t get_xtinst(SimulationTrap &e);

	PrivilegeLevel prepare_trap(SimulationTrap &e);

	void prepare_interrupt(const PendingInterrupts &x);

	PendingInterrupts compute_pending_interrupts();

	bool has_local_pending_enabled_interrupts() {
		return csrs.clint.mie.reg & csrs.clint.mip.reg;
	}

	void stsp_swap_sp_on_mode_change(PrivilegeLevel base_mode, PrivilegeLevel desc_mode);

	void swap_stack_pointer(uint32_t & new_sp);
	void verify_m_trap_vector(uint32_t mtvec_base_addr);
	void jump_to_trap_vector(PrivilegeLevel base_mode);
	uint32_t nv_mode_get_ivt_line_num(PrivilegeLevel base_mode);

	void return_from_trap_handler(PrivilegeLevel return_mode);

	void switch_to_trap_handler(PrivilegeLevel target_mode);

	void performance_and_sync_update(Opcode::Mapping executed_op);

	void run_step() override;

	void run() override;

	void show();
};

/* Do not call the run function of the ISS directly but use one of the Runner
 * wrappers. */
struct DirectCoreRunner : public sc_core::sc_module {
	ISS &core;
	std::string thread_name;

	SC_HAS_PROCESS(DirectCoreRunner);

	DirectCoreRunner(ISS &core) : sc_module(sc_core::sc_module_name(core.systemc_name.c_str())), core(core) {
		thread_name = "run" + std::to_string(core.get_hart_id());
		SC_NAMED_THREAD(run, thread_name.c_str());
	}

	void run() {
		core.run();

		if (core.status == CoreExecStatus::HitBreakpoint) {
			throw std::runtime_error(
			    "Breakpoints are not supported in the direct runner, use the debug "
			    "runner instead.");
		}
		assert(core.status == CoreExecStatus::Terminated);

		sc_core::sc_stop();
	}
};

}  // namespace rv32
