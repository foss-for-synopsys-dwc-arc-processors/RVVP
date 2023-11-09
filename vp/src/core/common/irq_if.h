#ifndef RISCV_ISA_IRQ_IF_H
#define RISCV_ISA_IRQ_IF_H

#include <stdint.h>

typedef uint32_t PrivilegeLevel;

// mode[2] -> V bit (as in MPV / SPV)
// mode[1:0] - Priv bits (as in MPP / SPP)
constexpr uint32_t MachineMode = 0b011;
constexpr uint32_t SupervisorMode = 0b001; // Same for S and HS
constexpr uint32_t UserMode = 0b00;
constexpr uint32_t VirtualSupervisorMode = 0b101;
constexpr uint32_t VirtualUserMode = 0b100;
constexpr uint32_t NoneMode = -1;  // invalid sentinel to avoid passing a boolean alongside a privilege level

static inline uint32_t PrivilegeLevelToPP(PrivilegeLevel priv)
{
	return priv & 0b011;
}

static inline uint32_t PrivilegeLevelToV(PrivilegeLevel priv)
{
	return (priv & 0b100) >> 2;
}

static inline PrivilegeLevel VPPToPrivilegeLevel(uint32_t V, uint32_t PP)
{
	// We don't have virtual machine mode
	if (PP == MachineMode)
		return PP;
	else
		return (V << 2) | PP;
}

static inline bool is_irq_capable_level(PrivilegeLevel level) {
	return level == MachineMode || level == SupervisorMode || level == VirtualSupervisorMode;
}

static inline const char * PrivilegeLevelToStr(PrivilegeLevel priv) {
	switch (priv) {
		case MachineMode:
			return "'mode: MM'";
		case VirtualSupervisorMode:
			return "'mode: VS'";
		case SupervisorMode:
			return "'mode: HS'";
		case VirtualUserMode:
			return "'mode: VU'";
		case UserMode:
			return "'mode: UU'";
		case NoneMode:
			return "'mode: --'";
		default:
			return "'mode: ?!'";
	}
}

struct external_interrupt_target {
	virtual ~external_interrupt_target() {}

	virtual void trigger_external_interrupt(PrivilegeLevel level) = 0;
	virtual void clear_external_interrupt(PrivilegeLevel level) = 0;
};

struct clint_interrupt_target {
	virtual ~clint_interrupt_target() {}

	virtual void trigger_timer_interrupt(bool status, PrivilegeLevel sw_irq_type) = 0;
	virtual uint64_t get_xtimecmp_level_csr(PrivilegeLevel level) = 0;
	virtual bool is_timer_compare_level_exists(PrivilegeLevel level) = 0;
	virtual void trigger_software_interrupt(bool status, PrivilegeLevel sw_irq_type) = 0;
};

struct interrupt_gateway {
	virtual ~interrupt_gateway() {}

	virtual void gateway_trigger_interrupt(uint32_t irq_id) = 0;
};

#endif  // RISCV_ISA_IRQ_IF_H
