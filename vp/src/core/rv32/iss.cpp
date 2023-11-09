#include "iss.h"

#include <tuple>

// to save *cout* format setting, see *ISS::show*
#include <boost/io/ios_state.hpp>
// for safe down-cast
#include <boost/lexical_cast.hpp>

using namespace rv32;

#define RAISE_ILLEGAL_INSTRUCTION() raise_trap(EXC_ILLEGAL_INSTR, instr.data());

#define REQUIRE_ISA(X)          \
    if (!(csrs.misa.reg & X))   \
        RAISE_ILLEGAL_INSTRUCTION()

#define RD instr.rd()
#define RS1 instr.rs1()
#define RS2 instr.rs2()
#define RS3 instr.rs3()

const char *regnames[] = {
	"zero (x0)", "ra   (x1)", "sp   (x2)", "gp   (x3)", "tp   (x4)", "t0   (x5)", "t1   (x6)", "t2   (x7)",
	"s0/fp(x8)", "s1   (x9)", "a0  (x10)", "a1  (x11)", "a2  (x12)", "a3  (x13)", "a4  (x14)", "a5  (x15)",
	"a6  (x16)", "a7  (x17)", "s2  (x18)", "s3  (x19)", "s4  (x20)", "s5  (x21)", "s6  (x22)", "s7  (x23)",
	"s8  (x24)", "s9  (x25)", "s10 (x26)", "s11 (x27)", "t3  (x28)", "t4  (x29)", "t5  (x30)", "t6  (x31)",
};

int regcolors[] = {
#if defined(COLOR_THEME_DARK)
	0,  1,  2,  3,  4,  5,  6,  52, 8,  9,  53, 54, 55, 56, 57, 58,
	16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
#elif defined(COLOR_THEME_LIGHT)
	100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 153, 154, 155, 156, 157, 158,
	116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131,
#else
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#endif
};

RegFile::RegFile() {
	memset(regs, 0, sizeof(regs));
}

RegFile::RegFile(const RegFile &other) {
	memcpy(regs, other.regs, sizeof(regs));
}

void RegFile::write(uint32_t index, int32_t value) {
	assert(index <= x31);
	assert(index != x0);
	regs[index] = value;
}

int32_t RegFile::read(uint32_t index) {
	if (index > x31)
		throw std::out_of_range("out-of-range register access");
	return regs[index];
}

uint32_t RegFile::shamt(uint32_t index) {
	assert(index <= x31);
	return BIT_RANGE(regs[index], 4, 0);
}

int32_t &RegFile::operator[](const uint32_t idx) {
	return regs[idx];
}

#if defined(COLOR_THEME_LIGHT) || defined(COLOR_THEME_DARK)
#define COLORFRMT "\e[38;5;%um%s\e[39m"
#define COLORPRINT(fmt, data) fmt, data
#else
#define COLORFRMT "%s"
#define COLORPRINT(fmt, data) data
#endif

void RegFile::show() {
	for (unsigned i = 0; i < NUM_REGS; ++i) {
		printf(COLORFRMT " = %8x\n", COLORPRINT(regcolors[i], regnames[i]), regs[i]);
	}
}

ISS::ISS(uint32_t hart_id, bool use_E_base_isa) : imsic("ImsicMem", hart_id, this), systemc_name("Core-" + std::to_string(hart_id)) {
	csrs.mhartid.reg = hart_id;
	if (use_E_base_isa)
		csrs.misa.select_E_base_isa();

	sc_core::sc_time qt = tlm::tlm_global_quantum::instance().get();
	cycle_time = sc_core::sc_time(10, sc_core::SC_NS);

	assert(qt >= cycle_time);
	assert(qt % cycle_time == sc_core::SC_ZERO_TIME);

	for (int i = 0; i < Opcode::NUMBER_OF_INSTRUCTIONS; ++i) instr_cycles[i] = cycle_time;

	const sc_core::sc_time memory_access_cycles = 4 * cycle_time;
	const sc_core::sc_time mul_div_cycles = 8 * cycle_time;

	instr_cycles[Opcode::LB] = memory_access_cycles;
	instr_cycles[Opcode::LBU] = memory_access_cycles;
	instr_cycles[Opcode::LH] = memory_access_cycles;
	instr_cycles[Opcode::LHU] = memory_access_cycles;
	instr_cycles[Opcode::LW] = memory_access_cycles;
	instr_cycles[Opcode::SB] = memory_access_cycles;
	instr_cycles[Opcode::SH] = memory_access_cycles;
	instr_cycles[Opcode::SW] = memory_access_cycles;
	instr_cycles[Opcode::MUL] = mul_div_cycles;
	instr_cycles[Opcode::MULH] = mul_div_cycles;
	instr_cycles[Opcode::MULHU] = mul_div_cycles;
	instr_cycles[Opcode::MULHSU] = mul_div_cycles;
	instr_cycles[Opcode::DIV] = mul_div_cycles;
	instr_cycles[Opcode::DIVU] = mul_div_cycles;
	instr_cycles[Opcode::REM] = mul_div_cycles;
	instr_cycles[Opcode::REMU] = mul_div_cycles;
	op = Opcode::UNDEF;
}

void ISS::hs_inst_check_access(void) {
	REQUIRE_ISA(H_ISA_EXT);

	if (prv == VirtualSupervisorMode || prv == VirtualUserMode)
		raise_trap(EXC_VIRTUAL_INSTRUCTION, instr.data());

	if (prv == UserMode && !csrs.hstatus.fields.hu)
		raise_trap(EXC_ILLEGAL_INSTR, instr.data());

	// TODO: Note that hypervisor Virtual-Machine Load and Store instructions
	// logic is valid only for SPMP/SMPU but not for MMU!
	assert((csrs.satp.fields.mode != SATP_MODE_BARE ||
		csrs.hgatp.fields.mode != SATP_MODE_BARE) && "instruction is not compatible with MMU");
}

inline PrivilegeLevel ISS::hs_inst_lvsv_mode(void) {
	return csrs.hstatus.fields.spvp ? VirtualSupervisorMode : VirtualUserMode;
}

void ISS::exec_step() {
	assert(((pc & ~pc_alignment_mask()) == 0) && "misaligned instruction");

	try {
		uint32_t mem_word = instr_mem->load_instr(pc);
		instr = Instruction(mem_word);
	} catch (SimulationTrap &e) {
		op = Opcode::UNDEF;
		instr = Instruction(0);
		throw;
	}

	if (instr.is_compressed()) {
		op = instr.decode_and_expand_compressed(RV32);
		pc += 2;
		if (op != Opcode::UNDEF)
			REQUIRE_ISA(C_ISA_EXT);
	} else {
		op = instr.decode_normal(RV32);
		pc += 4;
	}

	if (trace) {
		printf("core %2u: prv %1x: pc %8x: %s ", csrs.mhartid.reg, prv, last_pc, Opcode::mappingStr[op]);
		switch (Opcode::getType(op)) {
			case Opcode::Type::R:
				printf(COLORFRMT ", " COLORFRMT ", " COLORFRMT, COLORPRINT(regcolors[instr.rd()], regnames[instr.rd()]),
				       COLORPRINT(regcolors[instr.rs1()], regnames[instr.rs1()]),
				       COLORPRINT(regcolors[instr.rs2()], regnames[instr.rs2()]));
				break;
			case Opcode::Type::I:
				printf(COLORFRMT ", " COLORFRMT ", 0x%x", COLORPRINT(regcolors[instr.rd()], regnames[instr.rd()]),
				       COLORPRINT(regcolors[instr.rs1()], regnames[instr.rs1()]), instr.I_imm());
				break;
			case Opcode::Type::S:
				printf(COLORFRMT ", " COLORFRMT ", 0x%x", COLORPRINT(regcolors[instr.rs1()], regnames[instr.rs1()]),
				       COLORPRINT(regcolors[instr.rs2()], regnames[instr.rs2()]), instr.S_imm());
				break;
			case Opcode::Type::B:
				printf(COLORFRMT ", " COLORFRMT ", 0x%x", COLORPRINT(regcolors[instr.rs1()], regnames[instr.rs1()]),
				       COLORPRINT(regcolors[instr.rs2()], regnames[instr.rs2()]), instr.B_imm());
				break;
			case Opcode::Type::U:
				printf(COLORFRMT ", 0x%x", COLORPRINT(regcolors[instr.rd()], regnames[instr.rd()]), instr.U_imm());
				break;
			case Opcode::Type::J:
				printf(COLORFRMT ", 0x%x", COLORPRINT(regcolors[instr.rd()], regnames[instr.rd()]), instr.J_imm());
				break;
			case Opcode::Type::CSR:
				using namespace csr;
				printf("csr = 0x%x (%s", instr.csr(), csr_names.get_csr_name(instr.csr()));
				if (instr.csr() == MIREG_ADDR) {
					printf(" -> %s", icsr_names.get_icsr_name(csrs.miselect.reg));
				} else if (instr.csr() == SIREG_ADDR) {
					printf(" -> %s", icsr_names.get_icsr_name(csrs.siselect.reg));
				} else if (instr.csr() == VSIREG_ADDR) {
					printf(" -> %s", icsr_names.get_icsr_name(csrs.vsiselect.reg));
				}
				printf("), " COLORFRMT ", " COLORFRMT,
				       COLORPRINT(regcolors[instr.rd()], regnames[instr.rd()]),
				       COLORPRINT(regcolors[instr.rs1()], regnames[instr.rs1()]));
				break;
			case Opcode::Type::CSRI:
				printf("csr = 0x%x (%s), " COLORFRMT ", 0x%x", instr.csr(), csr_names.get_csr_name(instr.csr()),
				       COLORPRINT(regcolors[instr.rd()], regnames[instr.rd()]), instr.zimm());
				break;
			default:;
		}
		puts("");
	}

	switch (op) {
		case Opcode::UNDEF:
			if (trace)
				std::cout << "[ISS] WARNING: unknown instruction '" << std::to_string(instr.data()) << "' at address '"
				          << std::to_string(last_pc) << "'" << std::endl;
			raise_trap(EXC_ILLEGAL_INSTR, instr.data());
			break;

		case Opcode::ADDI:
			regs[instr.rd()] = regs[instr.rs1()] + instr.I_imm();
			break;

		case Opcode::SLTI:
			regs[instr.rd()] = regs[instr.rs1()] < instr.I_imm();
			break;

		case Opcode::SLTIU:
			regs[instr.rd()] = ((uint32_t)regs[instr.rs1()]) < ((uint32_t)instr.I_imm());
			break;

		case Opcode::XORI:
			regs[instr.rd()] = regs[instr.rs1()] ^ instr.I_imm();
			break;

		case Opcode::ORI:
			regs[instr.rd()] = regs[instr.rs1()] | instr.I_imm();
			break;

		case Opcode::ANDI:
			regs[instr.rd()] = regs[instr.rs1()] & instr.I_imm();
			break;

		case Opcode::ADD:
			regs[instr.rd()] = regs[instr.rs1()] + regs[instr.rs2()];
			break;

		case Opcode::SUB:
			regs[instr.rd()] = regs[instr.rs1()] - regs[instr.rs2()];
			break;

		case Opcode::SLL:
			regs[instr.rd()] = regs[instr.rs1()] << regs.shamt(instr.rs2());
			break;

		case Opcode::SLT:
			regs[instr.rd()] = regs[instr.rs1()] < regs[instr.rs2()];
			break;

		case Opcode::SLTU:
			regs[instr.rd()] = ((uint32_t)regs[instr.rs1()]) < ((uint32_t)regs[instr.rs2()]);
			break;

		case Opcode::SRL:
			regs[instr.rd()] = ((uint32_t)regs[instr.rs1()]) >> regs.shamt(instr.rs2());
			break;

		case Opcode::SRA:
			regs[instr.rd()] = regs[instr.rs1()] >> regs.shamt(instr.rs2());
			break;

		case Opcode::XOR:
			regs[instr.rd()] = regs[instr.rs1()] ^ regs[instr.rs2()];
			break;

		case Opcode::OR:
			regs[instr.rd()] = regs[instr.rs1()] | regs[instr.rs2()];
			break;

		case Opcode::AND:
			regs[instr.rd()] = regs[instr.rs1()] & regs[instr.rs2()];
			break;

		case Opcode::SLLI:
			regs[instr.rd()] = regs[instr.rs1()] << instr.shamt();
			break;

		case Opcode::SRLI:
			regs[instr.rd()] = ((uint32_t)regs[instr.rs1()]) >> instr.shamt();
			break;

		case Opcode::SRAI:
			regs[instr.rd()] = regs[instr.rs1()] >> instr.shamt();
			break;

		case Opcode::LUI:
			regs[instr.rd()] = instr.U_imm();
			break;

		case Opcode::AUIPC:
			regs[instr.rd()] = last_pc + instr.U_imm();
			break;

		case Opcode::JAL: {
			auto link = pc;
			pc = last_pc + instr.J_imm();
			trap_check_pc_alignment();
			regs[instr.rd()] = link;
		} break;

		case Opcode::JALR: {
			auto link = pc;
			pc = (regs[instr.rs1()] + instr.I_imm()) & ~1;
			trap_check_pc_alignment();
			regs[instr.rd()] = link;
		} break;

		case Opcode::SB: {
			uint32_t addr = regs[instr.rs1()] + instr.S_imm();
			mem->store_byte(addr, regs[instr.rs2()]);
		} break;

		case Opcode::SH: {
			uint32_t addr = regs[instr.rs1()] + instr.S_imm();
			trap_check_addr_alignment<2, false>(addr);
			mem->store_half(addr, regs[instr.rs2()]);
		} break;

		case Opcode::SW: {
			uint32_t addr = regs[instr.rs1()] + instr.S_imm();
			trap_check_addr_alignment<4, false>(addr);
			mem->store_word(addr, regs[instr.rs2()]);
		} break;

		case Opcode::LB: {
			uint32_t addr = regs[instr.rs1()] + instr.I_imm();
			regs[instr.rd()] = mem->load_byte(addr);
		} break;

		case Opcode::LH: {
			uint32_t addr = regs[instr.rs1()] + instr.I_imm();
			trap_check_addr_alignment<2, true>(addr);
			regs[instr.rd()] = mem->load_half(addr);
		} break;

		case Opcode::LW: {
			uint32_t addr = regs[instr.rs1()] + instr.I_imm();
			trap_check_addr_alignment<4, true>(addr);
			regs[instr.rd()] = mem->load_word(addr);
		} break;

		case Opcode::LBU: {
			uint32_t addr = regs[instr.rs1()] + instr.I_imm();
			regs[instr.rd()] = mem->load_ubyte(addr);
		} break;

		case Opcode::LHU: {
			uint32_t addr = regs[instr.rs1()] + instr.I_imm();
			trap_check_addr_alignment<2, true>(addr);
			regs[instr.rd()] = mem->load_uhalf(addr);
		} break;

		case Opcode::BEQ:
			if (regs[instr.rs1()] == regs[instr.rs2()]) {
				pc = last_pc + instr.B_imm();
				trap_check_pc_alignment();
			}
			break;

		case Opcode::BNE:
			if (regs[instr.rs1()] != regs[instr.rs2()]) {
				pc = last_pc + instr.B_imm();
				trap_check_pc_alignment();
			}
			break;

		case Opcode::BLT:
			if (regs[instr.rs1()] < regs[instr.rs2()]) {
				pc = last_pc + instr.B_imm();
				trap_check_pc_alignment();
			}
			break;

		case Opcode::BGE:
			if (regs[instr.rs1()] >= regs[instr.rs2()]) {
				pc = last_pc + instr.B_imm();
				trap_check_pc_alignment();
			}
			break;

		case Opcode::BLTU:
			if ((uint32_t)regs[instr.rs1()] < (uint32_t)regs[instr.rs2()]) {
				pc = last_pc + instr.B_imm();
				trap_check_pc_alignment();
			}
			break;

		case Opcode::BGEU:
			if ((uint32_t)regs[instr.rs1()] >= (uint32_t)regs[instr.rs2()]) {
				pc = last_pc + instr.B_imm();
				trap_check_pc_alignment();
			}
			break;

		case Opcode::FENCE:
		case Opcode::FENCE_I: {
			// not using out of order execution so can be ignored
		} break;

		case Opcode::ECALL: {
			if (sys) {
				sys->execute_syscall(this);
			} else {
				switch (prv) {
					case MachineMode:
						raise_trap(EXC_ECALL_M_MODE, last_pc);
						break;
					case VirtualSupervisorMode:
						raise_trap(EXC_ECALL_VS_MODE, last_pc);
						break;
					case SupervisorMode:
						raise_trap(EXC_ECALL_S_MODE, last_pc);
						break;
					case VirtualUserMode:
					case UserMode:
						raise_trap(EXC_ECALL_U_MODE, last_pc);
						break;
					default:
						throw std::runtime_error("unknown privilege level " + std::to_string(prv));
				}
			}
		} break;

		case Opcode::EBREAK: {
			// TODO: also raise trap and let the SW deal with it?
			status = CoreExecStatus::HitBreakpoint;
		} break;

		case Opcode::CSRRW: {
			auto addr = instr.csr();
			if (is_invalid_csr_access(addr, true)) {
				// TODO: it can be virtual instruction exception
				RAISE_ILLEGAL_INSTRUCTION();
			} else {
				auto rd = instr.rd();
				auto read = rd != RegFile::zero;
				auto rs1_val = regs[instr.rs1()];
				if (read) {
					regs[instr.rd()] = get_csr_value(addr);
				}
				set_csr_value(addr, rs1_val, read);
			}
		} break;

		case Opcode::CSRRS: {
			auto addr = instr.csr();
			auto rs1 = instr.rs1();
			auto write = rs1 != RegFile::zero;
			if (is_invalid_csr_access(addr, write)) {
				// TODO: it can be virtual instruction exception
				RAISE_ILLEGAL_INSTRUCTION();
			} else {
				auto rd = instr.rd();
				auto read = rd != RegFile::zero;
				auto rs1_val = regs[rs1];
				auto csr_val = get_csr_value(addr);
				if (read)
					regs[rd] = csr_val;
				if (write)
					set_csr_value(addr, csr_val | rs1_val, read);
			}
		} break;

		case Opcode::CSRRC: {
			auto addr = instr.csr();
			auto rs1 = instr.rs1();
			auto write = rs1 != RegFile::zero;
			if (is_invalid_csr_access(addr, write)) {
				// TODO: it can be virtual instruction exception
				RAISE_ILLEGAL_INSTRUCTION();
			} else {
				auto rd = instr.rd();
				auto read = rd != RegFile::zero;
				auto rs1_val = regs[rs1];
				auto csr_val = get_csr_value(addr);
				if (read)
					regs[rd] = csr_val;
				if (write)
					set_csr_value(addr, csr_val & ~rs1_val, read);
			}
		} break;

		case Opcode::CSRRWI: {
			auto addr = instr.csr();
			if (is_invalid_csr_access(addr, true)) {
				// TODO: it can be virtual instruction exception
				RAISE_ILLEGAL_INSTRUCTION();
			} else {
				auto rd = instr.rd();
				auto read = rd != RegFile::zero;
				if (read) {
					regs[rd] = get_csr_value(addr);
				}
				set_csr_value(addr, instr.zimm(), read);
			}
		} break;

		case Opcode::CSRRSI: {
			auto addr = instr.csr();
			auto zimm = instr.zimm();
			auto write = zimm != 0;
			if (is_invalid_csr_access(addr, write)) {
				// TODO: it can be virtual instruction exception
				RAISE_ILLEGAL_INSTRUCTION();
			} else {
				auto csr_val = get_csr_value(addr);
				auto rd = instr.rd();
				auto read = rd != RegFile::zero;
				if (read)
					regs[rd] = csr_val;
				if (write)
					set_csr_value(addr, csr_val | zimm, read);
			}
		} break;

		case Opcode::CSRRCI: {
			auto addr = instr.csr();
			auto zimm = instr.zimm();
			auto write = zimm != 0;
			if (is_invalid_csr_access(addr, write)) {
				// TODO: it can be virtual instruction exception
				RAISE_ILLEGAL_INSTRUCTION();
			} else {
				auto csr_val = get_csr_value(addr);
				auto rd = instr.rd();
				auto read = rd != RegFile::zero;
				if (read)
					regs[rd] = csr_val;
				if (write)
					set_csr_value(addr, csr_val & ~zimm, read);
			}
		} break;

		case Opcode::MUL: {
			REQUIRE_ISA(M_ISA_EXT);
			int64_t ans = (int64_t)regs[instr.rs1()] * (int64_t)regs[instr.rs2()];
			regs[instr.rd()] = ans & 0xFFFFFFFF;
		} break;

		case Opcode::MULH: {
			REQUIRE_ISA(M_ISA_EXT);
			int64_t ans = (int64_t)regs[instr.rs1()] * (int64_t)regs[instr.rs2()];
			regs[instr.rd()] = (ans & 0xFFFFFFFF00000000) >> 32;
		} break;

		case Opcode::MULHU: {
			REQUIRE_ISA(M_ISA_EXT);
			int64_t ans = ((uint64_t)(uint32_t)regs[instr.rs1()]) * (uint64_t)((uint32_t)regs[instr.rs2()]);
			regs[instr.rd()] = (ans & 0xFFFFFFFF00000000) >> 32;
		} break;

		case Opcode::MULHSU: {
			REQUIRE_ISA(M_ISA_EXT);
			int64_t ans = (int64_t)regs[instr.rs1()] * (uint64_t)((uint32_t)regs[instr.rs2()]);
			regs[instr.rd()] = (ans & 0xFFFFFFFF00000000) >> 32;
		} break;

		case Opcode::DIV: {
			REQUIRE_ISA(M_ISA_EXT);
			auto a = regs[instr.rs1()];
			auto b = regs[instr.rs2()];
			if (b == 0) {
				regs[instr.rd()] = -1;
			} else if (a == REG_MIN && b == -1) {
				regs[instr.rd()] = a;
			} else {
				regs[instr.rd()] = a / b;
			}
		} break;

		case Opcode::DIVU: {
			REQUIRE_ISA(M_ISA_EXT);
			auto a = regs[instr.rs1()];
			auto b = regs[instr.rs2()];
			if (b == 0) {
				regs[instr.rd()] = -1;
			} else {
				regs[instr.rd()] = (uint32_t)a / (uint32_t)b;
			}
		} break;

		case Opcode::REM: {
			REQUIRE_ISA(M_ISA_EXT);
			auto a = regs[instr.rs1()];
			auto b = regs[instr.rs2()];
			if (b == 0) {
				regs[instr.rd()] = a;
			} else if (a == REG_MIN && b == -1) {
				regs[instr.rd()] = 0;
			} else {
				regs[instr.rd()] = a % b;
			}
		} break;

		case Opcode::REMU: {
			REQUIRE_ISA(M_ISA_EXT);
			auto a = regs[instr.rs1()];
			auto b = regs[instr.rs2()];
			if (b == 0) {
				regs[instr.rd()] = a;
			} else {
				regs[instr.rd()] = (uint32_t)a % (uint32_t)b;
			}
		} break;

		case Opcode::LR_W: {
			REQUIRE_ISA(A_ISA_EXT);
			uint32_t addr = regs[instr.rs1()];
			trap_check_addr_alignment<4, true>(addr);
			regs[instr.rd()] = mem->atomic_load_reserved_word(addr);
			if (lr_sc_counter == 0)
				lr_sc_counter = 17;  // this instruction + 16 additional ones, (an over-approximation) to cover the RISC-V forward progress property
		} break;

		case Opcode::SC_W: {
			REQUIRE_ISA(A_ISA_EXT);
			uint32_t addr = regs[instr.rs1()];
			trap_check_addr_alignment<4, false>(addr);
			uint32_t val = regs[instr.rs2()];
			regs[instr.rd()] = 1;  // failure by default (in case a trap is thrown)
			regs[instr.rd()] = mem->atomic_store_conditional_word(addr, val) ? 0 : 1;  // overwrite result (in case no trap is thrown)
			lr_sc_counter = 0;
		} break;

		case Opcode::AMOSWAP_W: {
			REQUIRE_ISA(A_ISA_EXT);
			execute_amo(instr, [](int32_t a, int32_t b) {
				(void)a;
				return b;
			});
		} break;

		case Opcode::AMOADD_W: {
			REQUIRE_ISA(A_ISA_EXT);
			execute_amo(instr, [](int32_t a, int32_t b) { return a + b; });
		} break;

		case Opcode::AMOXOR_W: {
			REQUIRE_ISA(A_ISA_EXT);
			execute_amo(instr, [](int32_t a, int32_t b) { return a ^ b; });
		} break;

		case Opcode::AMOAND_W: {
			REQUIRE_ISA(A_ISA_EXT);
			execute_amo(instr, [](int32_t a, int32_t b) { return a & b; });
		} break;

		case Opcode::AMOOR_W: {
			REQUIRE_ISA(A_ISA_EXT);
			execute_amo(instr, [](int32_t a, int32_t b) { return a | b; });
		} break;

		case Opcode::AMOMIN_W: {
			REQUIRE_ISA(A_ISA_EXT);
			execute_amo(instr, [](int32_t a, int32_t b) { return std::min(a, b); });
		} break;

		case Opcode::AMOMINU_W: {
			REQUIRE_ISA(A_ISA_EXT);
			execute_amo(instr, [](int32_t a, int32_t b) { return std::min((uint32_t)a, (uint32_t)b); });
		} break;

		case Opcode::AMOMAX_W: {
			REQUIRE_ISA(A_ISA_EXT);
			execute_amo(instr, [](int32_t a, int32_t b) { return std::max(a, b); });
		} break;

		case Opcode::AMOMAXU_W: {
			REQUIRE_ISA(A_ISA_EXT);
			execute_amo(instr, [](int32_t a, int32_t b) { return std::max((uint32_t)a, (uint32_t)b); });
		} break;

			// RV32F Extension

		case Opcode::FLW: {
			REQUIRE_ISA(F_ISA_EXT);
			uint32_t addr = regs[instr.rs1()] + instr.I_imm();
			trap_check_addr_alignment<4, true>(addr);
			fp_regs.write(RD, float32_t{(uint32_t)mem->load_word(addr)});
		} break;

		case Opcode::FSW: {
			REQUIRE_ISA(F_ISA_EXT);
			uint32_t addr = regs[instr.rs1()] + instr.S_imm();
			trap_check_addr_alignment<4, false>(addr);
			mem->store_word(addr, fp_regs.u32(RS2));
		} break;

		case Opcode::FADD_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f32_add(fp_regs.f32(RS1), fp_regs.f32(RS2)));
			fp_finish_instr();
		} break;

		case Opcode::FSUB_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f32_sub(fp_regs.f32(RS1), fp_regs.f32(RS2)));
			fp_finish_instr();
		} break;

		case Opcode::FMUL_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f32_mul(fp_regs.f32(RS1), fp_regs.f32(RS2)));
			fp_finish_instr();
		} break;

		case Opcode::FDIV_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f32_div(fp_regs.f32(RS1), fp_regs.f32(RS2)));
			fp_finish_instr();
		} break;

		case Opcode::FSQRT_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f32_sqrt(fp_regs.f32(RS1)));
			fp_finish_instr();
		} break;

		case Opcode::FMIN_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();

			bool rs1_smaller = f32_lt_quiet(fp_regs.f32(RS1), fp_regs.f32(RS2)) ||
				(f32_eq(fp_regs.f32(RS1), fp_regs.f32(RS2)) && f32_isNegative(fp_regs.f32(RS1)));

			if (f32_isNaN(fp_regs.f32(RS1)) && f32_isNaN(fp_regs.f32(RS2))) {
				fp_regs.write(RD, f32_defaultNaN);
			} else {
				if (rs1_smaller)
					fp_regs.write(RD, fp_regs.f32(RS1));
				else
					fp_regs.write(RD, fp_regs.f32(RS2));
			}

			fp_finish_instr();
		} break;

		case Opcode::FMAX_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();

			bool rs1_greater = f32_lt_quiet(fp_regs.f32(RS2), fp_regs.f32(RS1)) ||
				(f32_eq(fp_regs.f32(RS2), fp_regs.f32(RS1)) && f32_isNegative(fp_regs.f32(RS2)));

			if (f32_isNaN(fp_regs.f32(RS1)) && f32_isNaN(fp_regs.f32(RS2))) {
				fp_regs.write(RD, f32_defaultNaN);
			} else {
				if (rs1_greater)
					fp_regs.write(RD, fp_regs.f32(RS1));
				else
					fp_regs.write(RD, fp_regs.f32(RS2));
			}

			fp_finish_instr();
		} break;

		case Opcode::FMADD_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f32_mulAdd(fp_regs.f32(RS1), fp_regs.f32(RS2), fp_regs.f32(RS3)));
			fp_finish_instr();
		} break;

		case Opcode::FMSUB_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f32_mulAdd(fp_regs.f32(RS1), fp_regs.f32(RS2), f32_neg(fp_regs.f32(RS3))));
			fp_finish_instr();
		} break;

		case Opcode::FNMADD_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f32_mulAdd(f32_neg(fp_regs.f32(RS1)), fp_regs.f32(RS2), f32_neg(fp_regs.f32(RS3))));
			fp_finish_instr();
		} break;

		case Opcode::FNMSUB_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f32_mulAdd(f32_neg(fp_regs.f32(RS1)), fp_regs.f32(RS2), fp_regs.f32(RS3)));
			fp_finish_instr();
		} break;

		case Opcode::FCVT_W_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			regs[RD] = f32_to_i32(fp_regs.f32(RS1), softfloat_roundingMode, true);
			fp_finish_instr();
		} break;

		case Opcode::FCVT_WU_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			regs[RD] = f32_to_ui32(fp_regs.f32(RS1), softfloat_roundingMode, true);
			fp_finish_instr();
		} break;

		case Opcode::FCVT_S_W: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, i32_to_f32(regs[RS1]));
			fp_finish_instr();
		} break;

		case Opcode::FCVT_S_WU: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, ui32_to_f32(regs[RS1]));
			fp_finish_instr();
		} break;

		case Opcode::FSGNJ_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			auto f1 = fp_regs.f32(RS1);
			auto f2 = fp_regs.f32(RS2);
			fp_regs.write(RD, float32_t{(f1.v & ~F32_SIGN_BIT) | (f2.v & F32_SIGN_BIT)});
			fp_set_dirty();
		} break;

		case Opcode::FSGNJN_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			auto f1 = fp_regs.f32(RS1);
			auto f2 = fp_regs.f32(RS2);
			fp_regs.write(RD, float32_t{(f1.v & ~F32_SIGN_BIT) | (~f2.v & F32_SIGN_BIT)});
			fp_set_dirty();
		} break;

		case Opcode::FSGNJX_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			auto f1 = fp_regs.f32(RS1);
			auto f2 = fp_regs.f32(RS2);
			fp_regs.write(RD, float32_t{f1.v ^ (f2.v & F32_SIGN_BIT)});
			fp_set_dirty();
		} break;

		case Opcode::FMV_W_X: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			fp_regs.write(RD, float32_t{(uint32_t)regs[RS1]});
			fp_set_dirty();
		} break;

		case Opcode::FMV_X_W: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			regs[RD] = fp_regs.u32(RS1);
		} break;

		case Opcode::FEQ_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			regs[RD] = f32_eq(fp_regs.f32(RS1), fp_regs.f32(RS2));
			fp_update_exception_flags();
		} break;

		case Opcode::FLT_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			regs[RD] = f32_lt(fp_regs.f32(RS1), fp_regs.f32(RS2));
			fp_update_exception_flags();
		} break;

		case Opcode::FLE_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			regs[RD] = f32_le(fp_regs.f32(RS1), fp_regs.f32(RS2));
			fp_update_exception_flags();
		} break;

		case Opcode::FCLASS_S: {
			REQUIRE_ISA(F_ISA_EXT);
			fp_prepare_instr();
			regs[RD] = f32_classify(fp_regs.f32(RS1));
		} break;

			// RV32D Extension

		case Opcode::FLD: {
			REQUIRE_ISA(D_ISA_EXT);
			uint32_t addr = regs[instr.rs1()] + instr.I_imm();
			trap_check_addr_alignment<8, true>(addr);
			fp_regs.write(RD, float64_t{(uint64_t)mem->load_double(addr)});
		} break;

		case Opcode::FSD: {
			REQUIRE_ISA(D_ISA_EXT);
			uint32_t addr = regs[instr.rs1()] + instr.S_imm();
			trap_check_addr_alignment<8, false>(addr);
			mem->store_double(addr, fp_regs.f64(RS2).v);
		} break;

		case Opcode::FADD_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f64_add(fp_regs.f64(RS1), fp_regs.f64(RS2)));
			fp_finish_instr();
		} break;

		case Opcode::FSUB_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f64_sub(fp_regs.f64(RS1), fp_regs.f64(RS2)));
			fp_finish_instr();
		} break;

		case Opcode::FMUL_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f64_mul(fp_regs.f64(RS1), fp_regs.f64(RS2)));
			fp_finish_instr();
		} break;

		case Opcode::FDIV_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f64_div(fp_regs.f64(RS1), fp_regs.f64(RS2)));
			fp_finish_instr();
		} break;

		case Opcode::FSQRT_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f64_sqrt(fp_regs.f64(RS1)));
			fp_finish_instr();
		} break;

		case Opcode::FMIN_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();

			bool rs1_smaller = f64_lt_quiet(fp_regs.f64(RS1), fp_regs.f64(RS2)) ||
				(f64_eq(fp_regs.f64(RS1), fp_regs.f64(RS2)) && f64_isNegative(fp_regs.f64(RS1)));

			if (f64_isNaN(fp_regs.f64(RS1)) && f64_isNaN(fp_regs.f64(RS2))) {
				fp_regs.write(RD, f64_defaultNaN);
			} else {
				if (rs1_smaller)
					fp_regs.write(RD, fp_regs.f64(RS1));
				else
					fp_regs.write(RD, fp_regs.f64(RS2));
			}

			fp_finish_instr();
		} break;

		case Opcode::FMAX_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();

			bool rs1_greater = f64_lt_quiet(fp_regs.f64(RS2), fp_regs.f64(RS1)) ||
				(f64_eq(fp_regs.f64(RS2), fp_regs.f64(RS1)) && f64_isNegative(fp_regs.f64(RS2)));

			if (f64_isNaN(fp_regs.f64(RS1)) && f64_isNaN(fp_regs.f64(RS2))) {
				fp_regs.write(RD, f64_defaultNaN);
			} else {
				if (rs1_greater)
					fp_regs.write(RD, fp_regs.f64(RS1));
				else
					fp_regs.write(RD, fp_regs.f64(RS2));
				}

			fp_finish_instr();
		} break;

		case Opcode::FMADD_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f64_mulAdd(fp_regs.f64(RS1), fp_regs.f64(RS2), fp_regs.f64(RS3)));
			fp_finish_instr();
		} break;

		case Opcode::FMSUB_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f64_mulAdd(fp_regs.f64(RS1), fp_regs.f64(RS2), f64_neg(fp_regs.f64(RS3))));
			fp_finish_instr();
		} break;

		case Opcode::FNMADD_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f64_mulAdd(f64_neg(fp_regs.f64(RS1)), fp_regs.f64(RS2), f64_neg(fp_regs.f64(RS3))));
			fp_finish_instr();
		} break;

		case Opcode::FNMSUB_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f64_mulAdd(f64_neg(fp_regs.f64(RS1)), fp_regs.f64(RS2), fp_regs.f64(RS3)));
			fp_finish_instr();
		} break;

		case Opcode::FSGNJ_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			auto f1 = fp_regs.f64(RS1);
			auto f2 = fp_regs.f64(RS2);
			fp_regs.write(RD, float64_t{(f1.v & ~F64_SIGN_BIT) | (f2.v & F64_SIGN_BIT)});
			fp_set_dirty();
		} break;

		case Opcode::FSGNJN_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			auto f1 = fp_regs.f64(RS1);
			auto f2 = fp_regs.f64(RS2);
			fp_regs.write(RD, float64_t{(f1.v & ~F64_SIGN_BIT) | (~f2.v & F64_SIGN_BIT)});
			fp_set_dirty();
		} break;

		case Opcode::FSGNJX_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			auto f1 = fp_regs.f64(RS1);
			auto f2 = fp_regs.f64(RS2);
			fp_regs.write(RD, float64_t{f1.v ^ (f2.v & F64_SIGN_BIT)});
			fp_set_dirty();
		} break;

		case Opcode::FCVT_S_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f64_to_f32(fp_regs.f64(RS1)));
			fp_finish_instr();
		} break;

		case Opcode::FCVT_D_S: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, f32_to_f64(fp_regs.f32(RS1)));
			fp_finish_instr();
		} break;

		case Opcode::FEQ_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			regs[RD] = f64_eq(fp_regs.f64(RS1), fp_regs.f64(RS2));
			fp_update_exception_flags();
		} break;

		case Opcode::FLT_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			regs[RD] = f64_lt(fp_regs.f64(RS1), fp_regs.f64(RS2));
			fp_update_exception_flags();
		} break;

		case Opcode::FLE_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			regs[RD] = f64_le(fp_regs.f64(RS1), fp_regs.f64(RS2));
			fp_update_exception_flags();
		} break;

		case Opcode::FCLASS_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			regs[RD] = (int64_t)f64_classify(fp_regs.f64(RS1));
		} break;

		case Opcode::FCVT_W_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			regs[RD] = f64_to_i32(fp_regs.f64(RS1), softfloat_roundingMode, true);
			fp_finish_instr();
		} break;

		case Opcode::FCVT_WU_D: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			regs[RD] = (int32_t)f64_to_ui32(fp_regs.f64(RS1), softfloat_roundingMode, true);
			fp_finish_instr();
		} break;

		case Opcode::FCVT_D_W: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, i32_to_f64((int32_t)regs[RS1]));
			fp_finish_instr();
		} break;

		case Opcode::FCVT_D_WU: {
			REQUIRE_ISA(D_ISA_EXT);
			fp_prepare_instr();
			fp_setup_rm();
			fp_regs.write(RD, ui32_to_f64((int32_t)regs[RS1]));
			fp_finish_instr();
		} break;

		// privileged instructions

		case Opcode::WFI:
			// NOTE: only a hint, can be implemented as NOP
			// std::cout << "[sim:wfi] CSR mstatus.mie " << csrs.mstatus->mie << std::endl;
			release_lr_sc_reservation();

			if (s_mode() && csrs.mstatus.fields.tw)
				raise_trap(EXC_ILLEGAL_INSTR, instr.data());

			if (vs_mode() && csrs.hstatus.fields.vtw)
				raise_trap(EXC_VIRTUAL_INSTRUCTION, instr.data());

			// When S-mode is implemented, then executing WFI in U-mode causes an illegal
			// instruction exception
			if (u_mode() && csrs.misa.has_supervisor_mode_extension())
				raise_trap(EXC_ILLEGAL_INSTR, instr.data());

			// TODO: has_local_pending_enabled_interrupts require injection modification
			if (!ignore_wfi && !has_local_pending_enabled_interrupts())
				sc_core::wait(wfi_event);
			break;

		case Opcode::SFENCE_VMA:
			if (s_mode() && csrs.mstatus.fields.tvm)
				raise_trap(EXC_ILLEGAL_INSTR, instr.data());
			if (vs_mode() && csrs.hstatus.fields.vtvm)
				raise_trap(EXC_VIRTUAL_INSTRUCTION, instr.data());
			mem->flush_tlb();
			break;

		case Opcode::SRET:
			if (!csrs.misa.has_supervisor_mode_extension())
				raise_trap(EXC_ILLEGAL_INSTR, instr.data());
			// TSR only affects mode
			if (s_mode() && csrs.mstatus.fields.tsr)
				raise_trap(EXC_ILLEGAL_INSTR, instr.data());
			if (vs_mode() && csrs.hstatus.fields.vtsr)
				raise_trap(EXC_VIRTUAL_INSTRUCTION, instr.data());

			if (prv == VirtualSupervisorMode)
				return_from_trap_handler(VirtualSupervisorMode);
			else if (prv == SupervisorMode)
				return_from_trap_handler(SupervisorMode);
			else if (prv == MachineMode)
				return_from_trap_handler(SupervisorMode);
			else
				throw std::runtime_error("SRET: unsupported privilege level " + std::to_string(prv));

			break;

		case Opcode::MRET:
			if (prv != MachineMode)
				raise_trap(EXC_ILLEGAL_INSTR, instr.data());

			return_from_trap_handler(MachineMode);
			break;

		case Opcode::HLVB: {
			hs_inst_check_access();
			uint32_t addr = regs[instr.rs1()];
			regs[instr.rd()] = mem->load_byte(addr, hs_inst_lvsv_mode());
		} break;

		case Opcode::HLVBU: {
			hs_inst_check_access();
			uint32_t addr = regs[instr.rs1()];
			regs[instr.rd()] = mem->load_ubyte(addr, hs_inst_lvsv_mode());
		} break;

		case Opcode::HLVH: {
			hs_inst_check_access();
			uint32_t addr = regs[instr.rs1()];
			trap_check_addr_alignment<2, true>(addr);
			regs[instr.rd()] = mem->load_half(addr, hs_inst_lvsv_mode());
		} break;

		case Opcode::HLVHU: {
			hs_inst_check_access();
			uint32_t addr = regs[instr.rs1()];
			trap_check_addr_alignment<2, true>(addr);
			regs[instr.rd()] = mem->load_uhalf(addr, hs_inst_lvsv_mode(), false);
		} break;

		case Opcode::HLVW: {
			hs_inst_check_access();
			uint32_t addr = regs[instr.rs1()];
			trap_check_addr_alignment<4, true>(addr);
			regs[instr.rd()] = mem->load_word(addr, hs_inst_lvsv_mode(), false);
		} break;

		case Opcode::HLVXHU: {
			hs_inst_check_access();
			uint32_t addr = regs[instr.rs1()];
			trap_check_addr_alignment<2, true>(addr);
			regs[instr.rd()] = mem->load_uhalf(addr, hs_inst_lvsv_mode(), true);
		} break;

		case Opcode::HLVXWU: {
			hs_inst_check_access();
			uint32_t addr = regs[instr.rs1()];
			trap_check_addr_alignment<4, true>(addr);
			regs[instr.rd()] = mem->load_word(addr, hs_inst_lvsv_mode(), true);
		} break;

		case Opcode::HSVB: {
			hs_inst_check_access();
			uint32_t addr = regs[instr.rs1()];
			mem->store_byte(addr, regs[instr.rs2()], hs_inst_lvsv_mode());
		} break;

		case Opcode::HSVH: {
			hs_inst_check_access();
			uint32_t addr = regs[instr.rs1()];
			trap_check_addr_alignment<2, false>(addr);
			mem->store_half(addr, regs[instr.rs2()], hs_inst_lvsv_mode());
		} break;

		case Opcode::HSVW: {
			hs_inst_check_access();
			uint32_t addr = regs[instr.rs1()];
			trap_check_addr_alignment<4, false>(addr);
			mem->store_word(addr, regs[instr.rs2()], hs_inst_lvsv_mode());
		} break;

		// instructions accepted by decoder but not by this RV32IMACF ISS -> do normal trap
		// RV64I
		case Opcode::LWU:
		case Opcode::LD:
		case Opcode::SD:
		case Opcode::ADDIW:
		case Opcode::SLLIW:
		case Opcode::SRLIW:
		case Opcode::SRAIW:
		case Opcode::ADDW:
		case Opcode::SUBW:
		case Opcode::SLLW:
		case Opcode::SRLW:
		case Opcode::SRAW:
		// RV64M
		case Opcode::MULW:
		case Opcode::DIVW:
		case Opcode::DIVUW:
		case Opcode::REMW:
		case Opcode::REMUW:
		// RV64A
		case Opcode::LR_D:
		case Opcode::SC_D:
		case Opcode::AMOSWAP_D:
		case Opcode::AMOADD_D:
		case Opcode::AMOXOR_D:
		case Opcode::AMOAND_D:
		case Opcode::AMOOR_D:
		case Opcode::AMOMIN_D:
		case Opcode::AMOMAX_D:
		case Opcode::AMOMINU_D:
		case Opcode::AMOMAXU_D:
		// RV64F
		case Opcode::FCVT_L_S:
		case Opcode::FCVT_LU_S:
		case Opcode::FCVT_S_L:
		case Opcode::FCVT_S_LU:
		// RV64D
		case Opcode::FCVT_L_D:
		case Opcode::FCVT_LU_D:
		case Opcode::FMV_X_D:
		case Opcode::FCVT_D_L:
		case Opcode::FCVT_D_LU:
		case Opcode::FMV_D_X:
			RAISE_ILLEGAL_INSTRUCTION();
			break;

		default:
			throw std::runtime_error("unknown opcode");
		}
}

uint64_t ISS::_compute_and_get_current_cycles() {
	assert(cycle_counter % cycle_time == sc_core::SC_ZERO_TIME);
	assert(cycle_counter.value() % cycle_time.value() == 0);

	uint64_t num_cycles = cycle_counter.value() / cycle_time.value();

	return num_cycles;
}


bool ISS::is_invalid_csr_access(uint32_t csr_addr, bool is_write) {
	if (csr_addr == csr::FFLAGS_ADDR || csr_addr == csr::FRM_ADDR || csr_addr == csr::FCSR_ADDR) {
		REQUIRE_ISA(F_ISA_EXT);
	}

	using namespace csr;

	// CSR Address [9:8]
	uint32_t csr_prv = (CSR_TYPE_MASK & csr_addr) >> CSR_TYPE_SHIFT;
	bool csr_readonly = ((0xC00 & csr_addr) >> 10) == 3;
	bool s_invalid = (csr_prv == SupervisorMode) && !csrs.misa.has_supervisor_mode_extension();
	bool u_invalid = (csr_prv == UserMode) && !csrs.misa.has_user_mode_extension();
	bool vs_invalid = (csr_prv == VirtualSupervisorMode) && !csrs.misa.has_hypervisor_mode_extension();

	/* CSR type fields match to priveledge bits in case of M/S/U modes */
	static_assert(CSR_TYPE_S == SupervisorMode);
	static_assert(CSR_TYPE_M == MachineMode);
	static_assert(CSR_TYPE_U == UserMode);

	bool priveledge_ok = false;

	if (csr_prv == CSR_TYPE_M) {
		priveledge_ok = m_mode();
	} else if (csr_prv == CSR_TYPE_S) {
		// VS accesses VS CSRs via S CSRs addresses
		priveledge_ok = (m_mode() || s_mode() || vs_mode());
	} else if (csr_prv == CSR_TYPE_HS_VS) {
		// both HS and VS CSRs can't be accessed by VS mode (and by other low-priveledge modes)
		priveledge_ok = (m_mode() || s_mode());
	} else if (csr_prv == CSR_TYPE_U) {
		// U and VU share same register bank - so everyone can access this bank.
		priveledge_ok = true;
	}

#if 0
	if (trace) {
		printf("[vp::rv32::csr] csr = 0x%x (%s), csr priveledge %u, readonly %u\n", csr_addr, csr_names.get_csr_name(csr_addr), csr_prv, csr_readonly);
	}
#endif

	return (is_write && csr_readonly) || (!priveledge_ok) || s_invalid || u_invalid || vs_invalid;
}


void ISS::validate_csr_counter_read_access_rights(uint32_t addr) {
	// match against counter CSR addresses, see RISC-V privileged spec for the address definitions
	if ((addr >= 0xC00 && addr <= 0xC1F) || (addr >= 0xC80 && addr <= 0xC9F)) {
		auto cnt = addr & 0x1F;  // 32 counter in total, naturally aligned with the mcounteren and scounteren CSRs

		if (s_mode() && !csr::is_bitset(csrs.mcounteren, cnt))
			RAISE_ILLEGAL_INSTRUCTION();

		if (u_mode() && (!csr::is_bitset(csrs.mcounteren, cnt) || !csr::is_bitset(csrs.scounteren, cnt)))
			RAISE_ILLEGAL_INSTRUCTION();
	}

	//TODO: handle VS mode?
}

uint32_t ISS::csr_address_virt_transform(uint32_t addr) {
	using namespace csr;

	// CSR Address [9:8]
	uint32_t csr_prv = (CSR_TYPE_MASK & addr) >> CSR_TYPE_SHIFT;

	/* In VS mode we access VS CSRs via S addresses */
	if (vs_mode() && (csr_prv == CSR_TYPE_S)) {
		return (addr & ~CSR_TYPE_MASK) | (CSR_TYPE_HS_VS << CSR_TYPE_SHIFT);
	}

	return addr;
}

void ISS::vs_csr_icsrs_access_exception(void) {
	if (prv == MachineMode || prv == SupervisorMode) {
		raise_trap(EXC_ILLEGAL_INSTR, instr.data());
	} else if (prv == VirtualSupervisorMode) {
		raise_trap(EXC_VIRTUAL_INSTRUCTION, instr.data());
	} else {
		/* all other priveleges should be discarded by is_invalid_csr_access() earlier */
		assert(false);
	}
}

void ISS::vstopei_access_check(void) {
	if (!csrs.hstatus.is_imsic_connected())
		vs_csr_icsrs_access_exception();
}

void ISS::vs_icsrs_access_check(unsigned icsr_addr) {
	bool valid_access = icsrs_vs.is_valid_addr(icsr_addr, csrs.hstatus.get_vgein());

	if (valid_access)
		return;

	vs_csr_icsrs_access_exception();
}

void ISS::s_sei_injection_access_check(void) {
	bool need_trap = (prv == SupervisorMode) &&
			 csrs.clint.is_iid_injected(SupervisorMode, EXC_S_EXTERNAL_INTERRUPT);

	if (need_trap) {
		raise_trap(EXC_ILLEGAL_INSTR, instr.data());
	}
}

void ISS::s_icsr_imsic_access_check(unsigned icsr_addr) {
	// If <EXC_S_EXTERNAL_INTERRUPT injected> attempts from S-mode to explicitly access the
	// supervisor-level interrupt file raise an illegal instruction exception. The exception
	// is raised for attempts to access CSR stopei, or to access sireg when siselect
	// has a value in the range 0x70–0xFF.
	if ((0x70 <= icsr_addr) && (icsr_addr <= 0xFF)) {
		s_sei_injection_access_check();
	}
}

void ISS::stimecmp_access_check(void) {
	using namespace csr;

	// When STCE in menvcfg is zero, an attempt to access stimecmp or vstimecmp in a mode other
	// than M-mode raises an illegal instruction exception
	if (!csrs.menvcfgh.fields.stce && !m_mode()) {
		raise_trap(EXC_ILLEGAL_INSTR, instr.data());
	}
}

void ISS::vstimecmp_access_check(void) {
	using namespace csr;

	// When STCE in menvcfg is zero, an attempt to access vstimecmp in a mode other
	// than M-mode raises an illegal instruction exception
	if (!csrs.menvcfgh.fields.stce && !m_mode()) {
		raise_trap(EXC_ILLEGAL_INSTR, instr.data());
	}

	// When STCE in menvcfg is one but STCE in henvcfg is zero, an attempt to access stimecmp
	// (really vstimecmp) when V = 1 raises a virtual instruction exception
	if (!csrs.henvcfgh.fields.stce && PrivilegeLevelToV(prv)) {
		raise_trap(EXC_VIRTUAL_INSTRUCTION, instr.data());
	}

	// check access from VirtualSupervisorMode when hvictl.vti = 1
	// NOTE: we check this last due to the wording:
	//  > Furthermore, for any given CSR, if there is some circumstance in which a write to the register
	//  > may cause a bit of vsip to change from one to zero, excluding bit 9 for external interrupts (SEIP),
	//  > then when VTI = 1, a virtual instruction exception is raised also for any attempt by the guest to
	//  > write this register
	// so, if vstimecmp/vstimecmph are not accessable in VirtualSupervisorMode - they
	// can't cause a bit of vsip to change from one to zero
	vs_clint_vti_access_check();
}

void ISS::vs_clint_vti_access_check(void) {
	// When bit VTI (Virtual Trap Interrupt control) = 1, attempts from VS-mode to explicitly access
	// CSRs sip and sie (or, for RV32 only, siph and sieh) cause a virtual instruction exception.
	// if register vstimecmp is implemented (from extension Sstc), then attempts from VS-mode to write to
	// stimecmp (or, for RV32 only, stimecmph) cause a virtual instruction exception when VTI = 1.

	if (!csrs.hvictl.is_vti_active())
		return;

	if (prv != VirtualSupervisorMode)
		return;

	raise_trap(EXC_VIRTUAL_INSTRUCTION, instr.data());
}

uint32_t ISS::get_csr_value(uint32_t addr) {
	validate_csr_counter_read_access_rights(addr);

	auto read = [=](auto &x, uint32_t mask) { return x.reg & mask; };

	addr = csr_address_virt_transform(addr);

	using namespace csr;

	switch (addr) {
		case csr_timecontrol::TIME_ADDR:{
			csrs.timecontrol.update_time_counter(clint->update_and_get_mtime());
			// time & timeh value depends on priveledge level (if V = 0 or 1)
			return csrs.timecontrol.read_time(PrivilegeLevelToV(prv));
		}

		case csr_timecontrol::TIMEH_ADDR:{
			csrs.timecontrol.update_time_counter(clint->update_and_get_mtime());
			// time & timeh value depends on priveledge level (if V = 0 or 1)
			return csrs.timecontrol.read_timeh(PrivilegeLevelToV(prv));
		}

		case CYCLE_ADDR:
			csrs.cycle.reg = _compute_and_get_current_cycles();
			return csrs.cycle.words.low;

		case CYCLEH_ADDR:
			return csrs.cycle.words.high;

		case MCYCLE_ADDR:
			csrs.cycle.reg = _compute_and_get_current_cycles();
			return csrs.cycle.words.low;

		case MCYCLEH_ADDR:
			csrs.cycle.reg = _compute_and_get_current_cycles();
			return csrs.cycle.words.high;

		case MINSTRET_ADDR:
			return csrs.instret.words.low;

		case MINSTRETH_ADDR:
			return csrs.instret.words.high;

		SWITCH_CASE_MATCH_ANY_HPMCOUNTER_RV32:  // not implemented
			return 0;

		case MSTATUS_ADDR:
			return read(csrs.mstatus, MSTATUS_MASK);
		case SSTATUS_ADDR:
			return read(csrs.mstatus, SSTATUS_MASK);

		case csrs_clint_pend::xip::MIP_ADDR:
			return csrs.clint.mip.checked_read_mip();
		case csrs_clint_pend::xip::SIP_ADDR:
			return csrs.clint.mip.checked_read_sip();
		case csrs_clint_pend::xip::HIP_ADDR:
			return csrs.clint.mip.checked_read_hip();
		case csrs_clint_pend::xip::VSIP_ADDR:
			vs_clint_vti_access_check();
			return csrs.clint.mip.checked_read_vsip();

		case csrs_clint_pend::xip::MIPH_ADDR:
			return csrs.clint.mip.checked_read_miph();
		case csrs_clint_pend::xip::SIPH_ADDR:
			return csrs.clint.mip.checked_read_siph();
		case csrs_clint_pend::xip::VSIPH_ADDR:
			vs_clint_vti_access_check();
			return csrs.clint.mip.checked_read_vsiph();

		case csrs_clint_pend::xie::MIE_ADDR:
			return csrs.clint.mie.checked_read_mie();
		case csrs_clint_pend::xie::SIE_ADDR:
			return csrs.clint.mie.checked_read_sie();
		case csrs_clint_pend::xie::HIE_ADDR:
			return csrs.clint.mie.checked_read_hie();
		case csrs_clint_pend::xie::VSIE_ADDR:
			vs_clint_vti_access_check();
			return csrs.clint.mie.checked_read_vsie();

		case csrs_clint_pend::xie::MIEH_ADDR:
			return csrs.clint.mie.checked_read_mieh();
		case csrs_clint_pend::xie::SIEH_ADDR:
			return csrs.clint.mie.checked_read_sieh();
		case csrs_clint_pend::xie::VSIEH_ADDR:
			vs_clint_vti_access_check();
			return csrs.clint.mie.checked_read_vsieh();

		case csrs_clint_pend::csrs_mvirt::MVIP_ADDR:
			return csrs.clint.mvirt.checked_read_mvip();
		case csrs_clint_pend::csrs_mvirt::MVIPH_ADDR:
			return csrs.clint.mvirt.checked_read_mviph();
		case csrs_clint_pend::csrs_mvirt::MVIEN_ADDR:
			return csrs.clint.mvirt.checked_read_mvien();
		case csrs_clint_pend::csrs_mvirt::MVIENH_ADDR:
			return csrs.clint.mvirt.checked_read_mvienh();

		case csrs_clint_pend::csrs_hvirt::HVIP_ADDR:
			return csrs.clint.hvirt.checked_read_hvip();
		case csrs_clint_pend::csrs_hvirt::HVIPH_ADDR:
			return csrs.clint.hvirt.checked_read_hviph();
		case csrs_clint_pend::csrs_hvirt::HVIEN_ADDR:
			return csrs.clint.hvirt.checked_read_hvien();
		case csrs_clint_pend::csrs_hvirt::HVIENH_ADDR:
			return csrs.clint.hvirt.checked_read_hvienh();

		case csrs_clint_pend::csr_hideleg::HIDELEG_ADDR:
			return csrs.clint.hideleg.checked_read_hideleg();
		case csrs_clint_pend::csr_hideleg::HIDELEGH_ADDR:
			return csrs.clint.hideleg.checked_read_hidelegh();

		case vs_iprio_banks::HVIPRIO1_ADDR:
			return icsrs_vs.iprio.hviprio1.checked_read(csrs.hstatus.get_vgein());
		case vs_iprio_banks::HVIPRIO1H_ADDR:
			return icsrs_vs.iprio.hviprio1h.checked_read(csrs.hstatus.get_vgein());
		case vs_iprio_banks::HVIPRIO2_ADDR:
			return icsrs_vs.iprio.hviprio2.checked_read(csrs.hstatus.get_vgein());
		case vs_iprio_banks::HVIPRIO2H_ADDR:
			return icsrs_vs.iprio.hviprio2h.checked_read(csrs.hstatus.get_vgein());

		case csr_hvictl::HVICTL_ADDR:
			return csrs.hvictl.checked_read();

		case csr_hgeie::HGEIE_ADDR:
			return csrs.hgeie.checked_read();

		case csr_hgeip::HGEIP_ADDR:
			return csrs.hgeip.checked_read();

		case csrs_clint_pend::csr_mideleg::MIDELEG_ADDR:
			return csrs.clint.mideleg.checked_read_mideleg();
		case csrs_clint_pend::csr_mideleg::MIDELEGH_ADDR:
			return csrs.clint.mideleg.checked_read_midelegh();

		case VSTOPI_ADDR:
			return get_vstopi_ipriom_adjusted();

		case STOPEI_ADDR:
			s_sei_injection_access_check();
			return csrs.default_read32(addr);

		case VSTOPEI_ADDR:
			vstopei_access_check();
			return csrs.default_read32(addr);

		case SATP_ADDR:
			if (csrs.mstatus.fields.tvm)
				RAISE_ILLEGAL_INSTRUCTION();
			break;

		case FCSR_ADDR:
			return read(csrs.fcsr, FCSR_MASK);

		case FFLAGS_ADDR:
			return csrs.fcsr.fields.fflags;

		case FRM_ADDR:
			return csrs.fcsr.fields.frm;

		// debug CSRs not supported, thus hardwired
		case TSELECT_ADDR:
			return 1; // if a zero write by SW is preserved, then debug mode is supported (thus hardwire to non-zero)
		case TDATA1_ADDR:
		case TDATA2_ADDR:
		case TDATA3_ADDR:
		case DCSR_ADDR:
		case DPC_ADDR:
		case DSCRATCH0_ADDR:
		case DSCRATCH1_ADDR:
			return 0;

		case csr_menvcfg::MENVCFG_ADDR:
			return csrs.menvcfg.checked_read();

		case csr_xenvcfgh::MENVCFGH_ADDR:
			return csrs.menvcfgh.checked_read();

		case csr_senvcfg::SENVCFG_ADDR:
			return csrs.senvcfg.checked_read();

		case csr_henvcfg::HENVCFG_ADDR:
			return csrs.henvcfg.checked_read();

		case csr_xenvcfgh::HENVCFGH_ADDR:
			return csrs.henvcfgh.checked_read();

		case csr_timecontrol::STIMECMP_ADDR:
			stimecmp_access_check();
			return csrs.timecontrol.stimecmp.words.low;
		case csr_timecontrol::STIMECMPH_ADDR:
			stimecmp_access_check();
			return csrs.timecontrol.stimecmp.words.high;

		case csr_timecontrol::VSTIMECMP_ADDR:
			vstimecmp_access_check();
			return csrs.timecontrol.vstimecmp.words.low;
		case csr_timecontrol::VSTIMECMPH_ADDR:
			vstimecmp_access_check();
			return csrs.timecontrol.vstimecmp.words.high;

		case MIREG_ADDR:
		case MIREG2_ADDR:
		case MIREG3_ADDR:
		case MIREG4_ADDR:
		case MIREG5_ADDR:
		case MIREG6_ADDR: {
			unsigned icsr_addr = csrs.miselect.reg + xireg_to_xselect_offset(addr);

			if (!icsrs_m.is_valid_addr(icsr_addr)) {
				RAISE_ILLEGAL_INSTRUCTION();
			}

			return icsrs_m.default_read32(icsr_addr);
		}

		case SISELECT_ADDR:
			return read(csrs.siselect, SISELECT_MASK);

		case SIREG_ADDR:
		case SIREG2_ADDR:
		case SIREG3_ADDR:
		case SIREG4_ADDR:
		case SIREG5_ADDR:
		case SIREG6_ADDR: {
			unsigned icsr_addr = csrs.siselect.reg + xireg_to_xselect_offset(addr);

			if (!icsrs_s.is_valid_addr(icsr_addr)) {
				RAISE_ILLEGAL_INSTRUCTION();
			}

			s_icsr_imsic_access_check(icsr_addr);

			return icsrs_s.default_read32(icsr_addr);
		}

		case VSISELECT_ADDR:
			return read(csrs.vsiselect, VSISELECT_MASK);

		case VSIREG_ADDR:
		case VSIREG2_ADDR:
		case VSIREG3_ADDR:
		case VSIREG4_ADDR:
		case VSIREG5_ADDR:
		case VSIREG6_ADDR: {
			unsigned icsr_addr = csrs.vsiselect.reg + xireg_to_xselect_offset(addr);

			vs_icsrs_access_check(icsr_addr);

			return icsrs_vs.default_read32(icsr_addr, csrs.hstatus.get_vgein());
		}

		case VSMPUMASK_ADDR:
			if (!csrs.hstatus.is_imsic_connected()) {
				raise_trap(EXC_ILLEGAL_INSTR, instr.data());
			}
			return csrs.default_read32(addr);
	}

	if (!csrs.is_valid_csr32_addr(addr))
		RAISE_ILLEGAL_INSTRUCTION();

	return csrs.default_read32(addr);
}

void ISS::set_csr_value(uint32_t addr, uint32_t value, bool read_accessed) {
	auto write = [=](auto &x, uint32_t mask) { x.reg = (x.reg & ~mask) | (value & mask); };

	addr = csr_address_virt_transform(addr);

	using namespace csr;

	switch (addr) {
		case MISA_ADDR:                         // currently, read-only, thus cannot be changed at runtime
		SWITCH_CASE_MATCH_ANY_HPMCOUNTER_RV32:  // not implemented
			break;

		case SATP_ADDR: {
			if (csrs.mstatus.fields.tvm)
				RAISE_ILLEGAL_INSTRUCTION();
			write(csrs.satp, SATP_MASK);
			// std::cout << "[iss] satp=" << boost::format("%x") % csrs.satp.reg << std::endl;
		} break;

		case MTVEC_ADDR:
			csrs.mtvec.checked_write(value);
			on_xtvec_write(MachineMode);
			break;
		case STVEC_ADDR:
			csrs.stvec.checked_write(value);
			on_xtvec_write(SupervisorMode);
			break;
		case VSTVEC_ADDR:
			csrs.vstvec.checked_write(value);
			on_xtvec_write(VirtualSupervisorMode);
			break;

		case MEPC_ADDR:
			write(csrs.mepc, pc_alignment_mask());
			break;
		case SEPC_ADDR:
			write(csrs.sepc, pc_alignment_mask());
			break;

		case MSTATUS_ADDR:
			write(csrs.mstatus, MSTATUS_MASK);
			break;
		case SSTATUS_ADDR:
			write(csrs.mstatus, SSTATUS_MASK);
			break;

		case MSTATUSH_ADDR:
			write(csrs.mstatush, MSTATUSH_MASK);
			break;

		case HSTATUS_ADDR:
			csrs.hstatus.checked_write(value);

			// proces everything we need to do after switching to new guest
			on_guest_switch();
			break;

		case csrs_clint_pend::xip::VSIP_ADDR:
		case csrs_clint_pend::xip::VSIPH_ADDR:
		case csrs_clint_pend::xie::VSIE_ADDR:
		case csrs_clint_pend::xie::VSIEH_ADDR:
			vs_clint_vti_access_check();
			[[fallthrough]];
		case csrs_clint_pend::xip::MIP_ADDR:
		case csrs_clint_pend::xip::SIP_ADDR:
		case csrs_clint_pend::xip::HIP_ADDR:
		case csrs_clint_pend::xip::MIPH_ADDR:
		case csrs_clint_pend::xip::SIPH_ADDR:
		case csrs_clint_pend::xie::MIE_ADDR:
		case csrs_clint_pend::xie::SIE_ADDR:
		case csrs_clint_pend::xie::HIE_ADDR:
		case csrs_clint_pend::xie::MIEH_ADDR:
		case csrs_clint_pend::xie::SIEH_ADDR:
		case csrs_clint_pend::csrs_mvirt::MVIP_ADDR:
		case csrs_clint_pend::csrs_mvirt::MVIPH_ADDR:
		case csrs_clint_pend::csrs_mvirt::MVIEN_ADDR:
		case csrs_clint_pend::csrs_mvirt::MVIENH_ADDR:
		case csrs_clint_pend::csrs_hvirt::HVIP_ADDR:
		case csrs_clint_pend::csrs_hvirt::HVIPH_ADDR:
		case csrs_clint_pend::csrs_hvirt::HVIEN_ADDR:
		case csrs_clint_pend::csrs_hvirt::HVIENH_ADDR:
		case csrs_clint_pend::csr_mideleg::MIDELEG_ADDR:
		case csrs_clint_pend::csr_mideleg::MIDELEGH_ADDR:
		case csrs_clint_pend::csr_hideleg::HIDELEG_ADDR:
		case csrs_clint_pend::csr_hideleg::HIDELEGH_ADDR:
			on_clint_csr_write(addr, value);
			break;

		case vs_iprio_banks::HVIPRIO1_ADDR:
			icsrs_vs.iprio.hviprio1.checked_write(value, csrs.hstatus.get_vgein());
			break;
		case vs_iprio_banks::HVIPRIO1H_ADDR:
			icsrs_vs.iprio.hviprio1h.checked_write(value, csrs.hstatus.get_vgein());
			break;
		case vs_iprio_banks::HVIPRIO2_ADDR:
			icsrs_vs.iprio.hviprio2.checked_write(value, csrs.hstatus.get_vgein());
			break;
		case vs_iprio_banks::HVIPRIO2H_ADDR:
			icsrs_vs.iprio.hviprio2h.checked_write(value, csrs.hstatus.get_vgein());
			break;

		case csr_hvictl::HVICTL_ADDR:
			csrs.hvictl.checked_write(value);
			break;

		case csr_hgeie::HGEIE_ADDR:
			csrs.hgeie.checked_write(value);

			// mip.sgeip depends on hgeie - so we need to recalc it after hgeie change
			recalc_sgeip();
			// TODO: do hgeie/hgeip behaves as delivery reg in NV mode? IOW, do we do extra route MSI to S-IMSIC now?
			break;

		case csr_hgeip::HGEIP_ADDR:
			csrs.hgeip.checked_write(value);
			// TODO: do hgeie/hgeip behaves as delivery reg in NV mode? IOW, do we do extra route MSI to S-IMSIC now?
			break;

		case MTOPEI_ADDR:
			claim_topei_interrupt_on_xtopei(MachineMode, value, !read_accessed);
			break;

		case STOPEI_ADDR:
			s_sei_injection_access_check();
			claim_topei_interrupt_on_xtopei(SupervisorMode, value, !read_accessed);
			break;

		case VSTOPEI_ADDR:
			vstopei_access_check();
			claim_topei_interrupt_on_xtopei(VirtualSupervisorMode, value, !read_accessed);
			break;

		case HEDELEG_ADDR:
			write(csrs.hedeleg, HEDELEG_MASK);
			break;

		case MCOUNTEREN_ADDR:
			write(csrs.mcounteren, MCOUNTEREN_MASK);
			break;

		case SCOUNTEREN_ADDR:
			write(csrs.scounteren, MCOUNTEREN_MASK);
			break;

		case MCOUNTINHIBIT_ADDR:
			write(csrs.mcountinhibit, MCOUNTINHIBIT_MASK);
			break;

		case FCSR_ADDR:
			write(csrs.fcsr, FCSR_MASK);
			break;

		case FFLAGS_ADDR:
			csrs.fcsr.fields.fflags = value;
			break;

		case FRM_ADDR:
			csrs.fcsr.fields.frm = value;
			break;

		// debug CSRs not supported, thus hardwired
		case TSELECT_ADDR:
		case TDATA1_ADDR:
		case TDATA2_ADDR:
		case TDATA3_ADDR:
		case DCSR_ADDR:
		case DPC_ADDR:
		case DSCRATCH0_ADDR:
		case DSCRATCH1_ADDR:
			break;

		case csr_menvcfg::MENVCFG_ADDR:
			csrs.menvcfg.checked_write(value);
			break;

		case csr_xenvcfgh::MENVCFGH_ADDR:
			csrs.menvcfgh.checked_write(value);
			on_xenvcfgh_write();
			break;

		case csr_senvcfg::SENVCFG_ADDR:
			csrs.senvcfg.checked_write(value);
			break;

		case csr_henvcfg::HENVCFG_ADDR:
			csrs.henvcfg.checked_write(value);
			break;

		case csr_xenvcfgh::HENVCFGH_ADDR:
			csrs.henvcfgh.checked_write(value);
			on_xenvcfgh_write();
			break;

		case csr_timecontrol::STIMECMP_ADDR:
			stimecmp_access_check();
			csrs.timecontrol.stimecmp.words.low = value;
			clint->post_write_xtimecmp();
			return;

		case csr_timecontrol::STIMECMPH_ADDR:
			stimecmp_access_check();
			csrs.timecontrol.stimecmp.words.high = value;
			clint->post_write_xtimecmp();
			return;

		case csr_timecontrol::VSTIMECMP_ADDR:
			vstimecmp_access_check();
			csrs.timecontrol.vstimecmp.words.low = value;
			clint->post_write_xtimecmp();
			return;

		case csr_timecontrol::VSTIMECMPH_ADDR:
			vstimecmp_access_check();
			csrs.timecontrol.vstimecmp.words.high = value;
			clint->post_write_xtimecmp();
			return;

		case MIREG_ADDR:
		case MIREG2_ADDR:
		case MIREG3_ADDR:
		case MIREG4_ADDR:
		case MIREG5_ADDR:
		case MIREG6_ADDR: {
			unsigned icsr_addr = csrs.miselect.reg + xireg_to_xselect_offset(addr);

			if (!icsrs_m.is_valid_addr(icsr_addr)) {
				RAISE_ILLEGAL_INSTRUCTION();
			}

			icsrs_m.default_write32(icsr_addr, value);

			// we need to re-calculate pendings after access to
			// - eie
			// - eip
			// - eithreshold
			// - eidelivery
			//
			// TODO: re-calculate only when access to registers above
			compute_imsic_pending_interrupts_m();

			return;
		}

		case SISELECT_ADDR:
			write(csrs.siselect, SISELECT_MASK);
			break;

		case SIREG_ADDR:
		case SIREG2_ADDR:
		case SIREG3_ADDR:
		case SIREG4_ADDR:
		case SIREG5_ADDR:
		case SIREG6_ADDR: {
			unsigned icsr_addr = csrs.siselect.reg + xireg_to_xselect_offset(addr);

			if (!icsrs_s.is_valid_addr(icsr_addr))
				RAISE_ILLEGAL_INSTRUCTION();

			s_icsr_imsic_access_check(icsr_addr);

			if (is_smpuaddr(icsr_addr) && csrs.smpumask.is_set_for_addr(icsr_addr))
				RAISE_ILLEGAL_INSTRUCTION();

			icsrs_s.default_write32(icsr_addr, value);

			compute_imsic_pending_interrupts_s();

			return;
		}

		case VSISELECT_ADDR:
			write(csrs.vsiselect, VSISELECT_MASK);
			break;

		case VSIREG_ADDR:
		case VSIREG2_ADDR:
		case VSIREG3_ADDR:
		case VSIREG4_ADDR:
		case VSIREG5_ADDR:
		case VSIREG6_ADDR: {
			unsigned icsr_addr = csrs.vsiselect.reg + xireg_to_xselect_offset(addr);

			vs_icsrs_access_check(icsr_addr);

			if (is_smpuaddr(icsr_addr) && csrs.vsmpumask.is_set_for_addr(icsr_addr)) {
				RAISE_ILLEGAL_INSTRUCTION();
			}

			icsrs_vs.default_write32(icsr_addr, csrs.hstatus.get_vgein(), value);

			compute_imsic_pending_interrupts_vs();

			return;
		}

		case VSMPUMASK_ADDR:
			if (!csrs.hstatus.is_imsic_connected()) {
				RAISE_ILLEGAL_INSTRUCTION();
			}
			csrs.default_write32(addr, value);
			return;

		case SPMPCFG0_ADDR:
		case SPMPCFG1_ADDR:
		case SPMPCFG2_ADDR:
		case SPMPCFG3_ADDR:
		case SPMPCFG4_ADDR:
		case SPMPCFG5_ADDR:
		case SPMPCFG6_ADDR:
		case SPMPCFG7_ADDR:
		case SPMPCFG8_ADDR:
		case SPMPCFG9_ADDR:
		case SPMPCFG10_ADDR:
		case SPMPCFG11_ADDR:
		case SPMPCFG12_ADDR:
		case SPMPCFG13_ADDR:
		case SPMPCFG14_ADDR:
		case SPMPCFG15_ADDR:
			mem->clear_spmp_cache();
			csrs.default_write32(addr, value);
			break;

		case SPMPADDR0_ADDR:
		case SPMPADDR1_ADDR:
		case SPMPADDR2_ADDR:
		case SPMPADDR3_ADDR:
		case SPMPADDR4_ADDR:
		case SPMPADDR5_ADDR:
		case SPMPADDR6_ADDR:
		case SPMPADDR7_ADDR:
		case SPMPADDR8_ADDR:
		case SPMPADDR9_ADDR:
		case SPMPADDR10_ADDR:
		case SPMPADDR11_ADDR:
		case SPMPADDR12_ADDR:
		case SPMPADDR13_ADDR:
		case SPMPADDR14_ADDR:
		case SPMPADDR15_ADDR:
		case SPMPADDR16_ADDR:
		case SPMPADDR17_ADDR:
		case SPMPADDR18_ADDR:
		case SPMPADDR19_ADDR:
		case SPMPADDR20_ADDR:
		case SPMPADDR21_ADDR:
		case SPMPADDR22_ADDR:
		case SPMPADDR23_ADDR:
		case SPMPADDR24_ADDR:
		case SPMPADDR25_ADDR:
		case SPMPADDR26_ADDR:
		case SPMPADDR27_ADDR:
		case SPMPADDR28_ADDR:
		case SPMPADDR29_ADDR:
		case SPMPADDR30_ADDR:
		case SPMPADDR31_ADDR:
		case SPMPADDR32_ADDR:
		case SPMPADDR33_ADDR:
		case SPMPADDR34_ADDR:
		case SPMPADDR35_ADDR:
		case SPMPADDR36_ADDR:
		case SPMPADDR37_ADDR:
		case SPMPADDR38_ADDR:
		case SPMPADDR39_ADDR:
		case SPMPADDR40_ADDR:
		case SPMPADDR41_ADDR:
		case SPMPADDR42_ADDR:
		case SPMPADDR43_ADDR:
		case SPMPADDR44_ADDR:
		case SPMPADDR45_ADDR:
		case SPMPADDR46_ADDR:
		case SPMPADDR47_ADDR:
		case SPMPADDR48_ADDR:
		case SPMPADDR49_ADDR:
		case SPMPADDR50_ADDR:
		case SPMPADDR51_ADDR:
		case SPMPADDR52_ADDR:
		case SPMPADDR53_ADDR:
		case SPMPADDR54_ADDR:
		case SPMPADDR55_ADDR:
		case SPMPADDR56_ADDR:
		case SPMPADDR57_ADDR:
		case SPMPADDR58_ADDR:
		case SPMPADDR59_ADDR:
		case SPMPADDR60_ADDR:
		case SPMPADDR61_ADDR:
		case SPMPADDR62_ADDR:
		case SPMPADDR63_ADDR:
			mem->clear_spmp_cache();
			csrs.default_write32(addr, value);
			break;

		case SPMPSWITCH0_ADDR:
		case SPMPSWITCH1_ADDR:
			mem->clear_spmp_cache();
			csrs.default_write32(addr, value);
			break;

		default:
			if (!csrs.is_valid_csr32_addr(addr))
				RAISE_ILLEGAL_INSTRUCTION();

			csrs.default_write32(addr, value);
	}
}

void ISS::update_interrupt_mode(PrivilegeLevel level) {
	switch (level) {
		case MachineMode:
			icsrs_m.eithreshold.set_mode_snps_vectored(csrs.mtvec.fields.mode);
			break;

		case VirtualSupervisorMode:
			// TODO: how to implement it?
			// what should happen if we changed vstvec when IMSIC is disconnected?
			if (csrs.hstatus.is_imsic_connected()) {
				unsigned current_guest = csrs.hstatus.get_guest_id();

				icsrs_vs.bank[current_guest].eithreshold.set_mode_snps_vectored(csrs.vstvec.fields.mode);
			}
			break;

		case SupervisorMode:
			icsrs_s.eithreshold.set_mode_snps_vectored(csrs.stvec.fields.mode);
			break;

		default:
			assert(false);
	}
}

void ISS::on_xtvec_write(PrivilegeLevel level) {
	update_interrupt_mode(level);
}

void ISS::on_xenvcfgh_write(void) {
	// When STCE in menvcfg is zero:
	//  -  STCE in henvcfg is read-only zero
	//  -  STIP in mip and sip reverts to its defined behavior as if this extension is not implemented.
	if (csrs.menvcfgh.fields.stce) {
		csrs.henvcfgh.make_stce_present(true);
	} else {
		csrs.henvcfgh.make_stce_present(false);
		// TODO: mask stie
		// csrs.clint.mip.fields.stip = 0;
		csrs.clint.mip.hw_write_mip(EXC_S_TIMER_INTERRUPT, false);
	}

	// When STCE in menvcfg is one but STCE in henvcfg is zero:
	//  -  VSTIP in hip reverts to its defined behavior as if this extension is not implemented
	if (!csrs.henvcfgh.fields.stce) {
		// TODO: mask vstip
		// csrs.clint.mip.fields.vstip = 0;
		csrs.clint.mip.hw_write_mip(EXC_VS_TIMER_INTERRUPT, false);
	}
}

void ISS::init(instr_memory_if *instr_mem, data_memory_if *data_mem, clint_if *clint, uint32_t entrypoint,
               uint32_t sp) {
	this->instr_mem = instr_mem;
	this->mem = data_mem;
	this->clint = clint;
	regs[RegFile::sp] = sp;
	pc = entrypoint;

	iprio_icsr_access_adjust();
}

void ISS::sys_exit() {
	shall_exit = true;
}

unsigned ISS::get_syscall_register_index() {
	if (csrs.misa.has_E_base_isa())
		return RegFile::a5;
	else
		return RegFile::a7;
}


uint64_t ISS::read_register(unsigned idx) {
	return (uint32_t)regs.read(idx);    //NOTE: zero extend
}

void ISS::write_register(unsigned idx, uint64_t value) {
	// Since the value parameter in the function prototype is
	// a uint64_t, signed integer values (e.g. -1) get promoted
	// to values within this range. For example, -1 would be
	// promoted to (2**64)-1. As such, we cannot perform a
	// Boost lexical or numeric cast to uint32_t here as
	// these perform bounds checks. Instead, we perform a C
	// cast without bounds checks.
	regs.write(idx, (uint32_t)value);
}

uint64_t ISS::get_progam_counter(void) {
	return pc;
}

void ISS::block_on_wfi(bool block) {
	ignore_wfi = !block;
}

CoreExecStatus ISS::get_status(void) {
	return status;
}

void ISS::set_status(CoreExecStatus s) {
	status = s;
}

void ISS::enable_debug(void) {
	debug_mode = true;
}

void ISS::insert_breakpoint(uint64_t addr) {
	breakpoints.insert(addr);
}

void ISS::remove_breakpoint(uint64_t addr) {
	breakpoints.erase(addr);
}

uint64_t ISS::get_hart_id() {
	return csrs.mhartid.reg;
}

std::vector<uint64_t> ISS::get_registers(void) {
	std::vector<uint64_t> regvals;

	for (auto v : regs.regs)
		regvals.push_back((uint32_t)v); //NOTE: zero extend

	return regvals;
}


void ISS::fp_finish_instr() {
	fp_set_dirty();
	fp_update_exception_flags();
}

void ISS::fp_prepare_instr() {
	assert(softfloat_exceptionFlags == 0);
	fp_require_not_off();
}

void ISS::fp_set_dirty() {
	csrs.mstatus.fields.sd = 1;
	csrs.mstatus.fields.fs = FS_DIRTY;
}

void ISS::fp_update_exception_flags() {
	if (softfloat_exceptionFlags) {
		fp_set_dirty();
		csrs.fcsr.fields.fflags |= softfloat_exceptionFlags;
		softfloat_exceptionFlags = 0;
	}
}

void ISS::fp_setup_rm() {
	auto rm = instr.frm();
	if (rm == FRM_DYN)
		rm = csrs.fcsr.fields.frm;
	if (rm >= FRM_RMM)
		RAISE_ILLEGAL_INSTRUCTION();
	softfloat_roundingMode = rm;
}

void ISS::fp_require_not_off() {
	if (csrs.mstatus.fields.fs == FS_OFF)
		RAISE_ILLEGAL_INSTRUCTION();
}

// called on xRET instruction
void ISS::return_from_trap_handler(PrivilegeLevel return_mode) {
	switch (return_mode) {
		case MachineMode:
			prv = VPPToPrivilegeLevel(csrs.mstatush.fields.mpv, csrs.mstatus.fields.mpp);
			csrs.mstatus.fields.mie = csrs.mstatus.fields.mpie;
			csrs.mstatus.fields.mpie = 1;
			pc = csrs.mepc.reg;
			csrs.mstatush.fields.mpv = 0;
			// An MRET or SRET instruction that changes the privilege mode to a mode less
			// privileged than M sets MPRV=0
			if (prv != MachineMode)
				csrs.mstatus.fields.mprv = 0;
			if (csrs.misa.has_user_mode_extension())
				csrs.mstatus.fields.mpp = UserMode;
			else
				csrs.mstatus.fields.mpp = MachineMode;
			break;

		case VirtualSupervisorMode:
			// we can only return to VS or VU mode from VS mode, so V=1 always
			prv = VPPToPrivilegeLevel(1, csrs.vsstatus.fields.spp);
			csrs.vsstatus.fields.sie = csrs.vsstatus.fields.spie;
			csrs.vsstatus.fields.spie = 1;
			pc = csrs.vsepc.reg;
			csrs.vsstatus.fields.spp = UserMode;
			csrs.mstatus.fields.mprv = 0; // sret can't return to M -> it always sets MPRV=0
			break;

		case SupervisorMode:
			prv = VPPToPrivilegeLevel(csrs.hstatus.fields.spv, csrs.mstatus.fields.spp);
			csrs.mstatus.fields.sie = csrs.mstatus.fields.spie;
			csrs.mstatus.fields.spie = 1;
			pc = csrs.sepc.reg;
			csrs.hstatus.fields.spv = 0;
			csrs.mstatus.fields.mprv = 0; // sret can't return to M -> it always sets MPRV=0
			if (csrs.misa.has_user_mode_extension())
				csrs.mstatus.fields.spp = UserMode;
			else
				csrs.mstatus.fields.spp = SupervisorMode;
			break;

		default:
			throw std::runtime_error("unknown privilege level " + std::to_string(return_mode));
	}

	stsp_swap_sp_on_mode_change(return_mode, prv);

	if (trace)
		printf("[vp::iss] return from trap handler, time %s, pc %8x, prv %s\n",
		       quantum_keeper.get_current_time().to_string().c_str(), pc, PrivilegeLevelToStr(prv));
}

void ISS::trigger_external_interrupt(PrivilegeLevel level) {
	// only AIA MSI supported
	assert(false);
}

void ISS::clear_external_interrupt(PrivilegeLevel level) {
	// only AIA MSI supported
	assert(false);
}

void ISS::trigger_timer_interrupt(bool status, PrivilegeLevel timer) {
	if (trace)
		std::cout << "[vp::iss] trigger " << PrivilegeLevelToStr(timer) << " timer interrupt=" << status << ", " << sc_core::sc_time_stamp() << std::endl;

	if (timer == MachineMode)
		clint_hw_irq_route(EXC_M_TIMER_INTERRUPT, status);
	else if (timer == SupervisorMode)
		clint_hw_irq_route(EXC_S_TIMER_INTERRUPT, status);
	else if (timer == VirtualSupervisorMode)
		clint_hw_irq_route(EXC_VS_TIMER_INTERRUPT, status);
	else
		assert(false);

	// TODO: do we need to call only on set?
	wfi_event.notify(sc_core::SC_ZERO_TIME);
}

bool ISS::is_timer_compare_level_exists(PrivilegeLevel level) {
	assert(is_irq_capable_level(level));

	if (level == MachineMode) {
		return true;
	} else if (level == SupervisorMode) {
		return csrs.menvcfgh.fields.stce;
	} else {
		return csrs.henvcfgh.fields.stce;
	}
}

uint64_t ISS::get_xtimecmp_level_csr(PrivilegeLevel level) {
	assert(level == SupervisorMode || level == VirtualSupervisorMode);

	return csrs.timecontrol.get_timecmp_level_adjusted(level);
}

void ISS::trigger_software_interrupt(bool status, PrivilegeLevel sw_irq_type) {
	assert(is_valid_privilege_level(sw_irq_type));

	if (trace)
		std::cout << "[vp::iss] trigger " << PrivilegeLevelToStr(sw_irq_type) << " software interrupt=" << status << ", " << sc_core::sc_time_stamp() << std::endl;

	if (sw_irq_type == MachineMode)
		clint_hw_irq_route(EXC_M_SOFTWARE_INTERRUPT, status);
	else if (sw_irq_type == SupervisorMode)
		clint_hw_irq_route(EXC_S_SOFTWARE_INTERRUPT, status);
	else
		assert(false);

	// TODO: do we need to call only on set?
	wfi_event.notify(sc_core::SC_ZERO_TIME);
}

csr_mcause & ISS::get_xcause(PrivilegeLevel level) {
	assert(is_irq_capable_level(level));

	if (level == MachineMode) {
		return csrs.mcause;
	} else if (level == SupervisorMode) {
		return csrs.scause;
	} else {
		return csrs.vscause;
	}
}

csr_mtvec & ISS::get_xtvec(PrivilegeLevel level) {
	assert(is_irq_capable_level(level));

	if (level == MachineMode) {
		return csrs.mtvec;
	} else if (level == SupervisorMode) {
		return csrs.stvec;
	} else {
		return csrs.vstvec;
	}
}

csr_topei & ISS::get_topei(PrivilegeLevel level) {
	assert(is_irq_capable_level(level));

	if (level == MachineMode) {
		return csrs.mtopei;
	} else if (level == SupervisorMode) {
		return csrs.stopei;
	} else {
		return csrs.vstopei;
	}
}

void ISS::set_topei(PrivilegeLevel level, uint32_t eiid) {
	csr_topei & topei = get_topei(level);

	topei.fields.iid = eiid;
	topei.fields.iprio = eiid;
}

uint32_t ISS::get_vstopi_ipriom_adjusted(void) {
	if (csrs.hvictl.is_ipriom_full_mode()) {
		return csrs.vstopi.reg;
	}

	// if bit IPRIOM (IPRIO Mode) of hvictl is zero, IPRIO in vstopi is 1;
	csr_topi vstopi_adjusted;

	vstopi_adjusted.fields.iid = csrs.vstopi.fields.iid;
	vstopi_adjusted.fields.iprio = (csrs.vstopi.reg != 0) ? 1 : 0;

	return vstopi_adjusted.reg;
}

void ISS::claim_topei_interrupt_on_xtopei(PrivilegeLevel target_level, uint32_t value, bool access_write_only) {
	// use xtopei as an interface for eithreshold only if
	//  - we are in nested vectored mode
	//  - the write value to CSR was 0
	//  - we've accessed xtopei CSR with write only access
	bool mark_irq_handled = is_irq_mode_snps_nested_vectored(target_level) && (value == 0) && access_write_only;

	claim_topei_interrupt(target_level, mark_irq_handled);
}

void ISS::claim_topei_interrupt_internal(PrivilegeLevel target_level) {
	claim_topei_interrupt(target_level, false);
}

void ISS::claim_topei_interrupt(PrivilegeLevel target_level, bool mark_irq_handled) {
	assert(is_irq_capable_level(target_level));

	uint32_t topei_iid = get_topei(target_level).fields.iid;

	if (trace)
		printf("[vp::iss::imsic] claim via topei, mode %s minor iid %u\n", PrivilegeLevelToStr(target_level), topei_iid);

	// claim_topei_interrupt can be called even if topei = 0 (no interrupts) as it called on all topei writes
	// this situation (topei.fields.iid = 0) is specially handled in imsic_update_eip_bit()
	if (target_level == MachineMode) {
		if (mark_irq_handled) {
			icsrs_m.eithreshold.mark_irq_as_handled();
		} else {
			imsic_update_eip_bit(icsrs_m.eip, topei_iid, false);
		}
		compute_imsic_pending_interrupts_m();
	} else if (target_level == SupervisorMode) {
		if (mark_irq_handled) {
			icsrs_s.eithreshold.mark_irq_as_handled();
		} else {
			imsic_update_eip_bit(icsrs_s.eip, topei_iid, false);
		}
		compute_imsic_pending_interrupts_s();
	} else {
		if (csrs.hstatus.is_imsic_connected()) {
			unsigned current_guest = csrs.hstatus.get_guest_id();
			struct icsr_vs_table::bank & bank = icsrs_vs.bank[current_guest];

			if (mark_irq_handled) {
				bank.eithreshold.mark_irq_as_handled();
			} else {
				imsic_update_eip_bit(bank.eip, topei_iid, false);
			}
		}
		compute_imsic_pending_interrupts_vs();
	}
}

void ISS::imsic_update_eip_bit(icsr_32 *eip, uint32_t value, bool set_bit) {
	assert(is_upper_bound_valid_minor_iid(value));

	// IID = 0 is not supported
	if (value == 0)
		return;

	uint32_t eip_arr_idx = value / icsr_32::BITS_PER_CSR;
	uint32_t eip_bit = value % icsr_32::BITS_PER_CSR;

	if (trace)
		printf("[vp::iss::imsic] eip%u bit %u %s\n", eip_arr_idx, value, set_bit ? "set" : "cleared");

	if (set_bit)
		eip[eip_arr_idx].reg |= BIT(eip_bit);
	else
		eip[eip_arr_idx].reg &= ~(BIT(eip_bit));
}

std::tuple<bool, uint32_t> ISS::compute_imsic_pending(icsr_32 *eip, icsr_32 *eie, uint32_t size, icsr_eithreshold &eithreshold, icsr_eidelivery &eidelivery, bool nested_vectored) {
	bool irq_pending = false;
	uint32_t topei_val = 0;
	constexpr uint32_t NO_IRQ_PENDING = iss_config::IMSIC_MAX_IRQS * 2; // value over any valid IID

	if (!eidelivery.delivery_on())
		return {false, 0};

	uint32_t eithreshold_value = eithreshold.reg;

	assert(eithreshold_value < iss_config::IMSIC_MAX_IRQS);

	// When eithreshold is zero, all enabled interrupt identities contribute to signaling
	// interrupts from the interrupt file.
	if (eithreshold_value == 0)
		eithreshold_value = iss_config::IMSIC_MAX_IRQS;

	uint32_t active_iid_num = NO_IRQ_PENDING;
	uint32_t ext_pend = 0;

	for (unsigned i = 0; i < size; i++) {
		ext_pend = eip[i].reg & eie[i].reg;

		if (ext_pend) {
			active_iid_num = i * icsr_32::BITS_PER_CSR + __builtin_ctz(ext_pend);
			break;
		}
	}

	bool threshold_matched = active_iid_num < eithreshold_value;

	if (threshold_matched) {
		irq_pending = true;
	}

	// In nseted vectored mode we set topei even if irq is not active due to eithreshold
	if (threshold_matched || nested_vectored) {
		topei_val = active_iid_num;
	}

	return {irq_pending, topei_val};
}

void ISS::on_guest_switch(void) {
	update_interrupt_mode(VirtualSupervisorMode);
	compute_imsic_pending_interrupts_vs();
}

void ISS::compute_imsic_pending_interrupts_m(void) {
	// MachineMode
	bool nv = is_irq_mode_snps_nested_vectored(MachineMode);
	auto [irq_pending, topei_val] = compute_imsic_pending(icsrs_m.eip, icsrs_m.eie, icsrs_m.eip_eie_arr_size, icsrs_m.eithreshold, icsrs_m.eidelivery, nv);
	csrs.clint.mip.hw_write_mip(EXC_M_EXTERNAL_INTERRUPT, irq_pending);
	set_topei(MachineMode, topei_val);
}

void ISS::compute_imsic_pending_interrupts_s(void) {
	// SupervisorMode
	bool nv = is_irq_mode_snps_nested_vectored(SupervisorMode);
	auto [irq_pending, topei_val] = compute_imsic_pending(icsrs_s.eip, icsrs_s.eie, icsrs_s.eip_eie_arr_size, icsrs_s.eithreshold, icsrs_s.eidelivery, nv);
	csrs.clint.mip.hw_write_mip(EXC_S_EXTERNAL_INTERRUPT, irq_pending);
	set_topei(SupervisorMode, topei_val);
}

void ISS::compute_imsic_pending_interrupts_vs(void) {
	// VirtualSupervisorMode
	bool vs_pend[iss_config::MAX_GUEST];
	uint32_t vs_eiid[iss_config::MAX_GUEST];

	/* we have to compute all banks pendings to update hgeip */
	for (unsigned i = 0; i < iss_config::MAX_GUEST; i++) {
		bool nv = is_irq_mode_snps_nested_vectored(VirtualSupervisorMode);
		struct icsr_vs_table::bank & bank = icsrs_vs.bank[i];
		std::tie(vs_pend[i], vs_eiid[i]) = compute_imsic_pending(bank.eip, bank.eie, icsrs_vs.eip_eie_arr_size, bank.eithreshold, bank.eidelivery, nv);

		csrs.hgeip.set_guest_pending(i, vs_pend[i]);
	}

	// recalc sgeip based on hgeip update
	recalc_sgeip();

	if (csrs.hstatus.is_imsic_connected()) {
		unsigned current_guest = csrs.hstatus.get_guest_id();
		// set only for selected guest. On guest switch we need to re-calculate.
		csrs.clint.mip.hw_write_mip(EXC_VS_EXTERNAL_INTERRUPT, vs_pend[current_guest]);
		set_topei(VirtualSupervisorMode, vs_eiid[current_guest]);
	} else {
		set_topei(VirtualSupervisorMode, 0);

		/* if no guest bank is connected - there would be no VS external HW interrupts at all */
		csrs.clint.mip.hw_write_mip(EXC_VS_EXTERNAL_INTERRUPT, false);
	}
}

void ISS::clint_hw_irq_route(uint32_t iid, bool set) {
	assert(major_irq::is_valid(iid));

	PendingInterrupts old_pendings_0 = compute_clint_pending_irq_bits_per_level();

	// update bit in HW mip
	csrs.clint.mip.hw_write_mip(iid, set);

	if (trace)
		printf("[vp::iss] try to update hw mip[iid=%d] to %u\n", iid, set);

	deliver_clint_changes_to_imsics(old_pendings_0, iid);
}

void ISS::iprio_icsr_access_adjust(void) {
	icsrs_s.iprio.update_dynamic_presence(csrs.clint.s_irqs_present());
	icsrs_vs.iprio.update_dynamic_presence(csrs.clint.vs_irqs_present());
}

void ISS::on_clint_csr_write(uint32_t addr, uint32_t value) {
	PendingInterrupts old_pendings_0 = compute_clint_pending_irq_bits_per_level();

	switch (addr) {
		case csrs_clint_pend::xip::MIP_ADDR:
			csrs.clint.mip.checked_write_mip(value);
			break;
		case csrs_clint_pend::xip::SIP_ADDR:
			csrs.clint.mip.checked_write_sip(value);
			break;
		case csrs_clint_pend::xip::HIP_ADDR:
			csrs.clint.mip.checked_write_hip(value);
			break;
		case csrs_clint_pend::xip::VSIP_ADDR:
			csrs.clint.mip.checked_write_vsip(value);
			break;

		case csrs_clint_pend::xip::MIPH_ADDR:
			csrs.clint.mip.checked_write_miph(value);
			break;
		case csrs_clint_pend::xip::SIPH_ADDR:
			csrs.clint.mip.checked_write_siph(value);
			break;
		case csrs_clint_pend::xip::VSIPH_ADDR:
			csrs.clint.mip.checked_write_vsiph(value);
			break;


		case csrs_clint_pend::xie::MIE_ADDR:
			csrs.clint.mie.checked_write_mie(value);
			break;
		case csrs_clint_pend::xie::SIE_ADDR:
			csrs.clint.mie.checked_write_sie(value);
			break;
		case csrs_clint_pend::xie::HIE_ADDR:
			csrs.clint.mie.checked_write_hie(value);
			break;
		case csrs_clint_pend::xie::VSIE_ADDR:
			csrs.clint.mie.checked_write_vsie(value);
			break;

		case csrs_clint_pend::xie::MIEH_ADDR:
			csrs.clint.mie.checked_write_mieh(value);
			break;
		case csrs_clint_pend::xie::SIEH_ADDR:
			csrs.clint.mie.checked_write_sieh(value);
			break;
		case csrs_clint_pend::xie::VSIEH_ADDR:
			csrs.clint.mie.checked_write_vsieh(value);
			break;


		case csrs_clint_pend::csrs_mvirt::MVIP_ADDR:
			csrs.clint.mvirt.checked_write_mvip(value);
			break;
		case csrs_clint_pend::csrs_mvirt::MVIPH_ADDR:
			csrs.clint.mvirt.checked_write_mviph(value);
			break;
		case csrs_clint_pend::csrs_mvirt::MVIEN_ADDR:
			csrs.clint.mvirt.checked_write_mvien(value);
			break;
		case csrs_clint_pend::csrs_mvirt::MVIENH_ADDR:
			csrs.clint.mvirt.checked_write_mvienh(value);
			break;


		case csrs_clint_pend::csrs_hvirt::HVIP_ADDR:
			csrs.clint.hvirt.checked_write_hvip(value);
			break;
		case csrs_clint_pend::csrs_hvirt::HVIPH_ADDR:
			csrs.clint.hvirt.checked_write_hviph(value);
			break;
		case csrs_clint_pend::csrs_hvirt::HVIEN_ADDR:
			csrs.clint.hvirt.checked_write_hvien(value);
			break;
		case csrs_clint_pend::csrs_hvirt::HVIENH_ADDR:
			csrs.clint.hvirt.checked_write_hvienh(value);
			break;


		case csrs_clint_pend::csr_mideleg::MIDELEG_ADDR:
			csrs.clint.mideleg.checked_write_mideleg(value);
			break;
		case csrs_clint_pend::csr_mideleg::MIDELEGH_ADDR:
			csrs.clint.mideleg.checked_write_midelegh(value);
			break;

		case csrs_clint_pend::csr_hideleg::HIDELEG_ADDR:
			csrs.clint.hideleg.checked_write_hideleg(value);
			break;
		case csrs_clint_pend::csr_hideleg::HIDELEGH_ADDR:
			csrs.clint.hideleg.checked_write_hidelegh(value);
			break;

		default:
			assert(false);
	}

	// TODO: it's enough to call it on hvien/hideleg/mvien/mideleg writes
	iprio_icsr_access_adjust();

	deliver_clint_changes_to_imsics(old_pendings_0);
}

// deliver changes in CLINT state to IMSIC (if snps 3NV mode is selected)
void ISS::deliver_clint_changes_to_imsics(PendingInterrupts old_pendings_0) {
	PendingInterrupts pendings_1 = compute_clint_pending_irq_bits_per_level();

	for (unsigned int iid = 0; iid < major_irq::MAX_INTERRUPTS_NUM; iid++) {
		deliver_pending_to_imsic(old_pendings_0, pendings_1, iid);
	}

	cascade_pendings_to_imsics(pendings_1);
}

// deliver single bit change (i.e. hw mip set) in CLINT state to IMSIC (if snps 3NV mode is selected)
void ISS::deliver_clint_changes_to_imsics(PendingInterrupts old_pendings_0, uint32_t iid) {
	PendingInterrupts pendings_1 = compute_clint_pending_irq_bits_per_level();

	deliver_pending_to_imsic(old_pendings_0, pendings_1, iid);
	cascade_pendings_to_imsics(pendings_1);
}

void ISS::cascade_pendings_to_imsics(PendingInterrupts pendings_1) {
	// pendings_2 - only to see if VS ext IRQ begin to pend for S level (not VS!)
	// after previous routing
	PendingInterrupts pendings_2 = compute_clint_pending_irq_bits_per_level();
	// if VS ext IRQ is not delegated to VS level - than it will be written to S imsic here
	deliver_pending_to_imsic(pendings_1, pendings_2, EXC_VS_EXTERNAL_INTERRUPT);

	// pendings_3 - only to see if S ext IRQ begin to pend for M level (not S!)
	// after previous routing
	PendingInterrupts pendings_3 = compute_clint_pending_irq_bits_per_level();
	// if S ext IRQ is not delegated to S level - than it will be written to M imsic here
	deliver_pending_to_imsic(pendings_2, pendings_3, EXC_S_EXTERNAL_INTERRUPT);
}

void ISS::deliver_pending_to_imsic(PendingInterrupts prev_pendings, PendingInterrupts new_pendings, uint32_t iid) {
	PrivilegeLevel imsic_level;
	bool edge = false;

	if (new_pendings.m_pending & BIT(iid)) {
		imsic_level = MachineMode;
		edge = csrs_clint_pend::pendings_edge_detected(prev_pendings.m_pending, new_pendings.m_pending, iid);
	} else if (new_pendings.s_hs_pending & BIT(iid)) {
		imsic_level = SupervisorMode;
		edge = csrs_clint_pend::pendings_edge_detected(prev_pendings.s_hs_pending, new_pendings.s_hs_pending, iid);
	} else if (new_pendings.vs_pending & BIT(iid)) {
		imsic_level = VirtualSupervisorMode;
		edge = csrs_clint_pend::pendings_edge_detected(prev_pendings.vs_pending, new_pendings.vs_pending, iid);
	} else {
		return;
	}

	/* further routing for NV mode (clint msi to imsic) */
	if (!edge)
		return;

	if (iid == major_irq::get_external_iid(imsic_level))
		return;

	assert(iid != EXC_M_EXTERNAL_INTERRUPT);
	assert(iid == EXC_S_EXTERNAL_INTERRUPT ? imsic_level == MachineMode : true);
	assert(iid == EXC_VS_EXTERNAL_INTERRUPT ? imsic_level == SupervisorMode : true);

	if (!is_irq_mode_snps_nested_vectored(imsic_level))
		return;

	if (imsic_level == VirtualSupervisorMode && !csrs.hstatus.is_imsic_connected())
		return;

	if (trace)
		printf("[vp::iss] deliver major irq [iid=%d] to %s imsic\n", iid, PrivilegeLevelToStr(imsic_level));

	// TODO: VS shift here or not?
	uint32_t minor_iid = get_iprio(imsic_level, iid);
	unsigned current_guest = csrs.hstatus.get_guest_id();
	route_imsic_write(imsic_level, current_guest, minor_iid);
}

void ISS::route_imsic_write(PrivilegeLevel target_imsic, unsigned guest_index, uint32_t value) {
	if (trace)
		printf("[vp::iss::imsic] got write: %s, guest idx %u, value %u\n", PrivilegeLevelToStr(target_imsic), guest_index, value);

	assert(value < iss_config::IMSIC_MAX_IRQS);
	assert(guest_index < iss_config::MAX_GUEST);

	// IID = 0 is not supported
	if (value == 0)
		return;

	if (target_imsic == MachineMode) {
		imsic_update_eip_bit(icsrs_m.eip, value, true);
		compute_imsic_pending_interrupts_m();
	} else if (target_imsic == SupervisorMode) {
		imsic_update_eip_bit(icsrs_s.eip, value, true);
		compute_imsic_pending_interrupts_s();
	} else if (target_imsic == VirtualSupervisorMode) {
		imsic_update_eip_bit(icsrs_vs.bank[guest_index].eip, value, true);
		compute_imsic_pending_interrupts_vs();
	} else {
		assert(false);
	}

	// NOTE: even if we update bits im MIP (inside compute_imsic_pending_interrupts*) we don't need
	// to call clint_hw_irq_route() here as we don't need to route external irq to IMSIC

	wfi_event.notify(sc_core::SC_ZERO_TIME);
}

uint32_t ISS::get_xtinst(SimulationTrap &e) {
	// TODO: Add EXC_LOAD_ADDR_MISALIGNED and EXC_STORE_AMO_ADDR_MISALIGNED
	if (instr.is_compressed())
		return 0; // TODO: We don't support xtinst transformations for compressed instruction for now
	else if ((e.reason == EXC_LOAD_PAGE_FAULT || e.reason == EXC_STORE_AMO_PAGE_FAULT ||
		e.reason == EXC_LOAD_GUEST_PAGE_FAULT || e.reason == EXC_STORE_AMO_GUEST_PAGE_FAULT ||
		e.reason == EXC_LOAD_ACCESS_FAULT || e.reason == EXC_STORE_AMO_ACCESS_FAULT))
		return instr.get_xtinst();
	else
		return 0;
}

PrivilegeLevel ISS::prepare_trap(SimulationTrap &e) {
	// undo any potential pc update (for traps the pc should point to the originating instruction and not it's
	// successor)
	pc = last_pc;
	unsigned exc_bit = (1 << e.reason);

	// 1) machine mode execution takes any traps, independent of delegation setting
	// 2) non-delegated traps are processed in machine mode, independent of current execution mode
	if (prv == MachineMode || !(exc_bit & csrs.medeleg.reg)) {
		csrs.mcause.fields.interrupt = 0;
		csrs.mcause.fields.exception_code = e.reason;
		csrs.mtval.reg = boost::lexical_cast<uint32_t>(e.mtval);
		csrs.mtval2.reg = boost::lexical_cast<uint32_t>(e.mtval2_htval);
		csrs.mtinst.reg = get_xtinst(e);
		return MachineMode;
	}

	// see above machine mode comment
	// 1) S mode execution takes any traps (which where previously delegated to S), independent of HS delegation setting
	// 2) non-delegated to VS traps are processed in S mode, independent of current execution mode (S/VS/U/VU)
	if (prv == SupervisorMode || prv == UserMode || !(exc_bit & csrs.hedeleg.reg)) {
		assert(prv == SupervisorMode || prv == UserMode || prv == VirtualSupervisorMode || prv == VirtualUserMode);
		csrs.scause.fields.interrupt = 0;
		csrs.scause.fields.exception_code = e.reason;
		csrs.stval.reg = boost::lexical_cast<uint32_t>(e.mtval);
		csrs.htval.reg = boost::lexical_cast<uint32_t>(e.mtval2_htval);
		csrs.htinst.reg = get_xtinst(e);
		return SupervisorMode;
	}

	assert(prv == VirtualSupervisorMode || prv == VirtualUserMode);
	assert(exc_bit & csrs.medeleg.reg);
	assert(exc_bit & csrs.hedeleg.reg);
	csrs.vscause.fields.interrupt = 0;
	csrs.vscause.fields.exception_code = e.reason;
	csrs.vstval.reg = boost::lexical_cast<uint32_t>(e.mtval);
	return VirtualSupervisorMode;
}

struct irq_cprio ISS::get_external_cprio_generic(PrivilegeLevel level) {
	return irq_cprio(level, get_topei(level).fields.iid);
}

struct irq_cprio ISS::get_external_cprio_256(PrivilegeLevel level) {
	return irq_cprio(level, 256);
}

struct irq_cprio ISS::get_external_cprio(PrivilegeLevel level) {
	assert(is_irq_capable_level(level));

	// TODO: we need to handle it the special way if EXC_S_EXTERNAL_INTERRUPT
	// is not delegated to S and andled in M
	// TODO: generalize this
	if ((level == SupervisorMode) && csrs.clint.is_iid_injected(SupervisorMode, EXC_S_EXTERNAL_INTERRUPT)) {
		return get_external_cprio_256(level);
	}

	if (level == VirtualSupervisorMode) {
		if (csrs.hstatus.is_imsic_connected()) {
			if (get_topei(level).reg != 0) {
				return get_external_cprio_generic(level);
			}
		} else {
			if (csrs.hvictl.is_external_injected()) {
				return irq_cprio(level, csrs.hvictl.fields.iprio);
			}
		}

		return get_external_cprio_256(level);
	}

	return get_external_cprio_generic(level);
}

struct irq_cprio ISS::get_local_cprio(PrivilegeLevel level, uint32_t iid) {
	assert(is_irq_capable_level(level));

	if (level == VirtualSupervisorMode && csrs.hvictl.is_local_injected() && major_irq::transform_vs_to_s(iid) == csrs.hvictl.get_s_iid()) {
		// VS IIDs are shown as VS here
		return irq_cprio(level, iid, csrs.hvictl.get_prio(), csrs.hvictl.fields.dpr);
	}

	// NOTE: IID should match the level here,
	// so if the level is VS we need to pass EXC_VS_TIMER_INTERRUPT, not EXC_S_TIMER_INTERRUPT
	// NOTE: internal order for standard local VS interrupts will be taken from VS - but
	// it should be OK as we won't get real S interrupts in VS level
	struct irq_cprio cprio(level, iid, get_iprio(level, iid));

	return cprio;
}

uint8_t ISS::get_iprio(PrivilegeLevel level, uint32_t iid) {
	assert(is_irq_capable_level(level));

	if (level == MachineMode) {
		return icsrs_m.iprio.get_iprio(iid);
	} else if (level == SupervisorMode) {
		return icsrs_s.iprio.get_iprio(iid);
	} else {
		// NOTE: for VS level we have iprio available on S interrupt places,
		// so we need to convert VS iid to S iid
		iid = major_irq::transform_vs_to_s(iid);

		// NOTE: if vgein = 0 we have extra iprio bank for that
		return icsrs_vs.iprio[csrs.hstatus.get_vgein()].get_iprio(iid);
	}
}

// TODO: we need to check hvictl here in case of VS level
struct irq_cprio ISS::major_irq_to_prio(PrivilegeLevel target_level, uint32_t iid) {
	assert(major_irq::is_valid(iid));
	assert(is_irq_capable_level(target_level));

	if (target_level == MachineMode && iid == EXC_M_EXTERNAL_INTERRUPT)
		return get_external_cprio(MachineMode);

	if (target_level == SupervisorMode && iid == EXC_S_EXTERNAL_INTERRUPT)
		return get_external_cprio(SupervisorMode);

	if (target_level == VirtualSupervisorMode && iid == EXC_VS_EXTERNAL_INTERRUPT)
		return get_external_cprio(VirtualSupervisorMode);

	// NOTE: IID should match the level here,
	// so if the level is VS we need to pass EXC_VS_TIMER_INTERRUPT, not EXC_S_TIMER_INTERRUPT
	return get_local_cprio(target_level, iid);
}

// TODO: we can relax checks later
void ISS::sanitize_vs_external_pend(PendingInterrupts &irqs_pend) {
	constexpr uint64_t vs_ext_irq = BIT(EXC_VS_EXTERNAL_INTERRUPT);

	assert(!(irqs_pend.m_pending & vs_ext_irq));

	// TODO: use is_iid_injected()
	if (!csrs.hstatus.is_imsic_connected() && !(csrs.clint.hvirt.hvip & BIT(EXC_VS_EXTERNAL_INTERRUPT))) {
		assert(!(irqs_pend.s_hs_pending & vs_ext_irq));
		assert(!(irqs_pend.vs_pending & vs_ext_irq));
	}

	// if nested vectored mode - only external irq should be checked among pendings
	constexpr uint64_t non_ext_irqs = ~(BIT(EXC_M_EXTERNAL_INTERRUPT) | BIT(EXC_S_EXTERNAL_INTERRUPT) | BIT(EXC_VS_EXTERNAL_INTERRUPT));

	if (is_irq_mode_snps_nested_vectored(MachineMode)) {
		assert(!(irqs_pend.m_pending & non_ext_irqs));
	}

	if (is_irq_mode_snps_nested_vectored(SupervisorMode)) {
		assert(!(irqs_pend.s_hs_pending & non_ext_irqs));
	}

	if (is_irq_mode_snps_nested_vectored(VirtualSupervisorMode)) {
		assert(!(irqs_pend.vs_pending & non_ext_irqs));
	}
}

std::tuple<uint32_t, uint8_t> ISS::major_irq_prepare_iid_prio(PrivilegeLevel level, uint64_t levels_pending) {
	struct irq_cprio cprio(irq_cprio::LOWEST_NONEXISTING_CPRIO);
	uint32_t iid = 0;

	for (uint32_t i = 0; i < major_irq::MAX_INTERRUPTS_NUM; i++) {
		if (levels_pending & BIT(i)) {
			struct irq_cprio new_cprio = major_irq_to_prio(level, i);

			if (new_cprio < cprio) {
				cprio = new_cprio;
				iid = i;
			}
		}
	}

	if (cprio.is_non_existing())
		throw std::runtime_error("some pending interrupt must be available here");

	// NOTE: IID is in M-level format (can't be put to vstopi directly)
	return {iid, cprio.to_iprio()};
}

// need to be called on:
//  - hgeie change
//  - hgeip change, which implies
//    - any change in any VS imsic
void ISS::recalc_sgeip(void) {
	uint32_t pending = csrs.hgeip.checked_read() & csrs.hgeie.checked_read();

	clint_hw_irq_route(EXC_S_GUEST_EXTERNAL_INTERRUPT, pending != 0);
}

void ISS::recalc_xtopi(PendingInterrupts &irqs_pend) {
	sanitize_vs_external_pend(irqs_pend);

	if (irqs_pend.m_pending) {
		auto [exc, iprio] = major_irq_prepare_iid_prio(MachineMode, irqs_pend.m_pending);
		csrs.mtopi.fields.iid = exc;
		csrs.mtopi.fields.iprio = iprio;
	} else {
		csrs.mtopi.reg = 0;
	}

	if (irqs_pend.s_hs_pending) {
		auto [exc, iprio] = major_irq_prepare_iid_prio(SupervisorMode, irqs_pend.s_hs_pending);
		csrs.stopi.fields.iid = exc;
		csrs.stopi.fields.iprio = iprio;
	} else {
		csrs.stopi.reg = 0;
	}

	if (irqs_pend.vs_pending) {
		auto [exc, iprio] = major_irq_prepare_iid_prio(VirtualSupervisorMode, irqs_pend.vs_pending);

		// place VS interrupts on S places
		exc = major_irq::transform_vs_to_s(exc);

		csrs.vstopi.fields.iid = exc;
		csrs.vstopi.fields.iprio = iprio;
	} else {
		csrs.vstopi.reg = 0;
	}
}

bool ISS::is_irq_mode_snps_nested_vectored(PrivilegeLevel target_mode) {
	return get_xtvec(target_mode).fields.mode == csr_mtvec::Mode::SnpsNestedVectored;
}

// called on trap caused by interrupts
void ISS::notify_irq_taken(PrivilegeLevel target_mode) {
	uint32_t topei_iid = get_topei(target_mode).fields.iid;

	// update eithreshold if required. each eithreshold holds info if it needs to react or not (based on mode)
	// claim corresponding interrupt in IMSIC
	// TODO: this function can be cleaned up significantly
	switch (target_mode) {
		case MachineMode:
			if (is_irq_mode_snps_nested_vectored(target_mode)) {
				icsrs_m.eithreshold.update_with_new_irq(topei_iid);
				claim_topei_interrupt_internal(target_mode);
			}
			break;

		case SupervisorMode:
			if (is_irq_mode_snps_nested_vectored(target_mode) && !csrs.clint.is_iid_injected(target_mode, EXC_S_EXTERNAL_INTERRUPT)) {
				icsrs_s.eithreshold.update_with_new_irq(topei_iid);
				claim_topei_interrupt_internal(target_mode);
			}
			break;

		case VirtualSupervisorMode:
			if (csrs.hstatus.is_imsic_connected() && is_irq_mode_snps_nested_vectored(target_mode)) {
				unsigned current_guest = csrs.hstatus.get_guest_id();
				icsrs_vs.bank[current_guest].eithreshold.update_with_new_irq(topei_iid);
				claim_topei_interrupt_internal(target_mode);
			}
			break;

		default:
			assert(false);
	}
}

PrivilegeLevel ISS::compute_pending_interrupt(PendingInterrupts &irqs_pend) {
	if (irqs_pend.m_pending && is_irq_globaly_enable_per_level(MachineMode)) {
		return MachineMode;
	} else if (irqs_pend.s_hs_pending && is_irq_globaly_enable_per_level(SupervisorMode)) {
		return SupervisorMode;
	} else if (irqs_pend.vs_pending && is_irq_globaly_enable_per_level(VirtualSupervisorMode)) {
		return VirtualSupervisorMode;
	} else {
		return NoneMode;
	}
}

std::tuple<PrivilegeLevel, bool> ISS::prepare_interrupt(void) {
	PendingInterrupts irqs_pend = compute_clint_pending_irq_bits_per_level();

	irqs_pend = process_clint_pending_irq_bits_per_level(irqs_pend);

	// TODO: we can move recalc_xtopi to the place where we change mip bits
	recalc_xtopi(irqs_pend);

	PrivilegeLevel target_mode = compute_pending_interrupt(irqs_pend);

	if (target_mode == NoneMode)
		return {target_mode, false};

	if (trace) {
		uint32_t iid = -1;

		if (target_mode == MachineMode)
			iid = csrs.mtopi.fields.iid;
		else if (target_mode == SupervisorMode)
			iid = csrs.stopi.fields.iid;
		else if (target_mode == VirtualSupervisorMode)
			iid = csrs.vstopi.fields.iid;
		std::cout << "[vp::iss] prepare interrupt, target-mode=" << PrivilegeLevelToStr(target_mode)
		          << ", major iid=" << std::dec << iid << std::endl;
		if (iid == EXC_M_EXTERNAL_INTERRUPT)
		    std::cout << "[vp::iss] eiid=" << csrs.mtopei.fields.iid << std::endl;
		else if (iid == EXC_S_EXTERNAL_INTERRUPT)
		    std::cout << "[vp::iss] eiid=" << csrs.stopei.fields.iid << std::endl;
		else if (iid == EXC_VS_EXTERNAL_INTERRUPT)
		    std::cout << "[vp::iss] eiid=" << csrs.vstopei.fields.iid << std::endl;
	}

	switch (target_mode) {
		case MachineMode:
			csrs.mcause.fields.exception_code = csrs.mtopi.fields.iid;
			csrs.mcause.fields.interrupt = 1;
			csrs.mtinst.reg = 0;
			break;

		case SupervisorMode:
			csrs.scause.fields.exception_code = csrs.stopi.fields.iid;
			csrs.scause.fields.interrupt = 1;
			csrs.htinst.reg = 0;
			break;

		case VirtualSupervisorMode:
			csrs.vscause.fields.exception_code = csrs.vstopi.fields.iid;
			csrs.vscause.fields.interrupt = 1;
			break;

		default:
			assert(false);
			break;
	}

	return {target_mode, true};
}

bool ISS::is_irq_globaly_enable_per_level(PrivilegeLevel target_mode) {
	switch (target_mode) {
		case MachineMode:
			return (prv != MachineMode || (prv == MachineMode && csrs.mstatus.fields.mie));

		case SupervisorMode:
			return ((prv != MachineMode && prv != SupervisorMode) || (prv == SupervisorMode && csrs.mstatus.fields.sie));

		case VirtualSupervisorMode:
			// note: VS interrupts global enable are control by vsstatus.sie which is not mstatus subset
			return ((prv == VirtualUserMode) || (prv == VirtualSupervisorMode && csrs.vsstatus.fields.sie));

		default:
			throw std::runtime_error("unexpected privilege level " + std::to_string(target_mode));
	}
}

struct PendingInterrupts ISS::compute_clint_pending_irq_bits_per_level(void) {
	uint64_t m_pending = csrs.clint.mie.reg & csrs.clint.mip.reg & ~csrs.clint.mideleg.checked_read_mideleg_64();

	uint64_t s_pending = csrs.clint.mip.sip_routed_read_64() & csrs.clint.mie.sie_routed_read_64();
	uint32_t hs_pending = csrs.clint.mip.checked_read_hip() & csrs.clint.mie.checked_read_hie();
	uint64_t s_hs_pending = (s_pending | hs_pending) & ~csrs.clint.hideleg.checked_read_hideleg_64();
	uint64_t vs_pending = csrs.clint.mip.vsip_routed_read_64() & csrs.clint.mie.vsie_routed_read_64();

	if (csrs.hvictl.is_local_injected()) {
		vs_pending &= BIT(EXC_VS_EXTERNAL_INTERRUPT);
		// NOTE: hvictl iid is in S format, (VS irqs are in S places, however we do all)
		// processing in M format (VS irqs in VS places), so we do backward tranformation first
		// However, it's only supported for real VS interrupts, which is not fully compatible
		// to spec. TODO: fix this
		auto [tranformable, iid] = major_irq::transform_s_to_vs(csrs.hvictl.get_s_iid());

		if (tranformable)
			vs_pending |= BIT(iid);
	}

	return {m_pending, s_hs_pending, vs_pending};
}

struct PendingInterrupts ISS::process_clint_pending_irq_bits_per_level(PendingInterrupts & pendings) {
	PendingInterrupts pendings_processed = pendings;

	// Adjust computed per-mode pendings with rules for IRQ mode
	// For the Direct anf Vectored modes we don't need the adjustment
	// For SnpsNestedVectored we need to mask everything except external interrupt of correspondent mode

	if (is_irq_mode_snps_nested_vectored(MachineMode)) {
		pendings_processed.m_pending &= BIT(EXC_M_EXTERNAL_INTERRUPT);
	}

	if (is_irq_mode_snps_nested_vectored(SupervisorMode)) {
		pendings_processed.s_hs_pending &= BIT(EXC_S_EXTERNAL_INTERRUPT);
	}

	if (is_irq_mode_snps_nested_vectored(VirtualSupervisorMode)) {
		pendings_processed.vs_pending &= BIT(EXC_VS_EXTERNAL_INTERRUPT);
	}

	return pendings_processed;
}

void ISS::swap_stack_pointer(uint32_t & new_sp) {
	uint32_t temp = new_sp;
	new_sp = (uint32_t)regs[RegFile::sp];
	regs[RegFile::sp] = (int32_t)temp;
}

// base_mode - mode where xRET is executed or mode which we trap to
// desc_mode - mode where we appear after xRET or mode which we trap from
void ISS::stsp_swap_sp_on_mode_change(PrivilegeLevel base_mode, PrivilegeLevel desc_mode) {
	switch (base_mode) {
		case MachineMode:
			if (csrs.menvcfg.fields.mtsp) {
				if (desc_mode != MachineMode) {
					swap_stack_pointer(csrs.mtsp.reg);
				}
			}

			break;

		case VirtualSupervisorMode:
			if (csrs.henvcfg.fields.vgtsp) {
				if (desc_mode != VirtualSupervisorMode) {
					swap_stack_pointer(csrs.vstsp.reg);
				}
			}

			break;

		case SupervisorMode:
			if (csrs.henvcfg.fields.htsp) {
				if (PrivilegeLevelToV(desc_mode)) {
					swap_stack_pointer(csrs.htsp.reg);
				}
			}

			if (csrs.senvcfg.fields.stsp) {
				if (!PrivilegeLevelToV(desc_mode) && desc_mode != SupervisorMode) {
					swap_stack_pointer(csrs.stsp.reg);
				}
			}

			break;

		default:
			assert(false);
	}
}

void ISS::verify_m_trap_vector(uint32_t mtvec_base_addr) {
	if (mtvec_base_addr == 0) {
		if(error_on_zero_traphandler) {
			throw std::runtime_error("[ISS] Took null trap handler in machine mode");
		} else {
			static bool once = true;
			if (once)
				std::cout << "[ISS] Warn: Taking trap handler in machine mode to 0x0, this is probably an error." << std::endl;
			once = false;
		}
	}
}

void ISS::set_pending_ivt(uint32_t address) {
	ivt_access.pending = true;
	ivt_access.entry_address = address;
}

void ISS::process_pending_ivt(void) {
	if (ivt_access.pending) {
		ivt_access.pending = false;

		if (trace) {
			printf("[vp::iss] do postponed IVT fetch access, t-prv %s\n", PrivilegeLevelToStr(prv));
		}

		// TODO: this need to be switched to implicit memory access accessor
		pc = mem->load_word(ivt_access.entry_address);
	}
}

uint32_t ISS::nv_mode_get_ivt_line_num(PrivilegeLevel base_mode) {
	uint32_t xtopei = get_topei(base_mode).fields.iid;

	// TODO: generalize this
	if (base_mode == SupervisorMode && csrs.clint.is_iid_injected(SupervisorMode, EXC_S_EXTERNAL_INTERRUPT)) {
		xtopei = 256;
	}

	assert(xtopei);
	static_assert(iss_config::NV_MODE_MAX_VECTOR <= iss_config::IMSIC_MAX_IRQS);

	if (xtopei >= iss_config::NV_MODE_MAX_VECTOR)
		return iss_config::NV_MODE_MAX_VECTOR;
	else
		return xtopei;
}

void ISS::jump_to_trap_vector(PrivilegeLevel base_mode) {
	assert(is_irq_capable_level(base_mode));

	constexpr uint32_t xtvec_ptr_size = 4;

	csr_mtvec & xtvec = get_xtvec(base_mode);
	csr_mcause & xcause = get_xcause(base_mode);
	uint32_t xtvec_base = xtvec.get_base_address();

	if (base_mode == MachineMode)
		verify_m_trap_vector(xtvec_base);

	bool is_interrupt = xcause.fields.interrupt;

	if (is_interrupt && xtvec.fields.mode == csr_mtvec::Mode::Vectored)
		pc = xtvec_base + xtvec_ptr_size * xcause.fields.exception_code;
	else if (is_interrupt && xtvec.fields.mode == csr_mtvec::Mode::SnpsNestedVectored)
		set_pending_ivt(xtvec_base + xtvec_ptr_size * nv_mode_get_ivt_line_num(base_mode));
	else if (!is_interrupt && xtvec.fields.mode == csr_mtvec::Mode::SnpsNestedVectored)
		set_pending_ivt(xtvec_base);
	else
		pc = xtvec_base;

	if (is_interrupt)
		notify_irq_taken(base_mode);
}

// target_mode - mode where trap handler will be called
void ISS::switch_to_trap_handler(PrivilegeLevel target_mode) {
	if (trace) {
		printf("[vp::iss] switch to trap handler, time %s, last_pc %8x, pc %8x, irq %u, t-prv %s\n",
		       quantum_keeper.get_current_time().to_string().c_str(), last_pc, pc, csrs.mcause.fields.interrupt, PrivilegeLevelToStr(target_mode));
	}

	// free any potential LR/SC bus lock before processing a trap/interrupt
	release_lr_sc_reservation();

	auto pp = prv;
	prv = target_mode;

	stsp_swap_sp_on_mode_change(target_mode, pp);

	switch (target_mode) {
		case MachineMode:
			csrs.mepc.reg = pc;

			csrs.mstatus.fields.mpie = csrs.mstatus.fields.mie;
			csrs.mstatus.fields.mie = 0;
			csrs.mstatus.fields.mpp = PrivilegeLevelToPP(pp);
			csrs.mstatush.fields.mpv = PrivilegeLevelToV(pp);

			break;

		case VirtualSupervisorMode:
			csrs.vsepc.reg = pc;

			csrs.vsstatus.fields.spie = csrs.vsstatus.fields.sie;
			csrs.vsstatus.fields.sie = 0;
			csrs.vsstatus.fields.spp = PrivilegeLevelToPP(pp);

			// we can process trap in VS mode only if it came from VS or VU
			assert(pp == VirtualSupervisorMode || pp == VirtualUserMode);

			// we don't need to store V status (where is no place to store it) - we always come from V

			break;

		case SupervisorMode:
			assert(pp == SupervisorMode || pp == VirtualSupervisorMode || pp == UserMode || pp == VirtualUserMode);

			csrs.sepc.reg = pc;

			csrs.mstatus.fields.spie = csrs.mstatus.fields.sie;
			csrs.mstatus.fields.sie = 0;
			csrs.mstatus.fields.spp = PrivilegeLevelToPP(pp);
			csrs.hstatus.fields.spv = PrivilegeLevelToV(pp);

			// When V=1 and a trap is taken into HS-mode, bit SPVP (Supervisor Previous Virtual Privilege)
			// is set to the nominal privilege mode at the time of the trap, the same as sstatus.SPP. But if
			// V=0 before a trap, SPVP is left unchanged on trap entry.
			if (PrivilegeLevelToV(pp)) {
				csrs.hstatus.fields.spvp = PrivilegeLevelToPP(pp);
			}

			break;

		default:
			throw std::runtime_error("unknown privilege level " + std::string(PrivilegeLevelToStr(target_mode)));
	}

	jump_to_trap_vector(target_mode);
}

void ISS::performance_and_sync_update(Opcode::Mapping executed_op) {
	++total_num_instr;

	if (!csrs.mcountinhibit.fields.IR)
		++csrs.instret.reg;

	if (lr_sc_counter != 0) {
		--lr_sc_counter;
		assert (lr_sc_counter >= 0);
		if (lr_sc_counter == 0)
		release_lr_sc_reservation();
	}

	auto new_cycles = instr_cycles[executed_op];

	if (!csrs.mcountinhibit.fields.CY)
		cycle_counter += new_cycles;

	quantum_keeper.inc(new_cycles);
	if (quantum_keeper.need_sync()) {
		if (lr_sc_counter == 0) // match SystemC sync with bus unlocking in a tight LR_W/SC_W loop
			quantum_keeper.sync();
	}
}

void ISS::run_step() {
	try {
		assert(regs.read(0) == 0);

		// process postponed IVT fetch. We can't do it directly in switch_to_trap_handler()
		// as we may need to catch instruction fetch violation exception recursively in
		// catch (SimulationTrap) section.
		process_pending_ivt();

		// speeds up the execution performance (non debug mode) significantly by
		// checking the additional flag first
		if (debug_mode && (breakpoints.find(pc) != breakpoints.end())) {
			status = CoreExecStatus::HitBreakpoint;
			return;
		}

		last_pc = pc;

		exec_step();

		auto [target_mode, need_switch_to_trap] = prepare_interrupt();
		// std::cout << "mip=0x" << std::hex << csrs.clint.mip.reg << " mie=0x" << csrs.mie.reg << " prv=" << PrivilegeLevelToStr(prv) << ((target_mode == NoneMode) ? " no irq" : " has irq") << std::endl;
		if (need_switch_to_trap) {
			switch_to_trap_handler(target_mode);
		}
	} catch (SimulationTrap &e) {
		if (trace)
			std::cout << "[vp::iss] take trap " << e.reason << " in mode " << PrivilegeLevelToStr(prv) << ", mtval=" << e.mtval << std::endl;
		auto target_mode = prepare_trap(e);
		switch_to_trap_handler(target_mode);
	}

	// NOTE: writes to zero register are supposedly allowed but must be ignored
	// (reset it after every instruction, instead of checking *rd != zero*
	// before every register write)
	regs.regs[regs.zero] = 0;

	// Do not use a check *pc == last_pc* here. The reason is that due to
	// interrupts *pc* can be set to *last_pc* accidentally (when jumping back
	// to *mepc*).
	if (shall_exit)
		status = CoreExecStatus::Terminated;

	performance_and_sync_update(op);
}

void ISS::run() {
	// run a single step until either a breakpoint is hit or the execution
	// terminates
	do {
		run_step();
	} while (status == CoreExecStatus::Runnable);

	// force sync to make sure that no action is missed
	quantum_keeper.sync();
}

void ISS::show() {
	boost::io::ios_flags_saver ifs(std::cout);
	std::cout << "=[ core : " << csrs.mhartid.reg << " ]===========================" << std::endl;
	std::cout << "simulation time: " << sc_core::sc_time_stamp() << std::endl;
	regs.show();
	std::cout << "pc = " << std::hex << pc << std::endl;
	std::cout << "num-instr = " << std::dec << csrs.instret.reg << std::endl;
}
