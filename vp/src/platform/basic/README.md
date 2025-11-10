# Basic single-core RV32 platform

## Interrupt controllers

Platform supports RTIA and UIA UCLIC interrupt controllers in a single riscv-vp executable.

__NOTE__: the AIA/RTIA features (IMSIC, RTIA nested vectored mode, major interrupt priorities, major interrupt injection, etc.) and UIA UCLIC are mutually exclusive. The platform should be treated as one of the following configurations:
- RTIA platform (interrupt delivery from UCLIC should not be enabled)
- UIA UCLIC platform (RTIA nested vectored mode and interrupt delivery from IMSIC should not be enabled, and major interrupt injection should not be used)

### UIA UCLIC

#### Features and configuration

- 2 domains: Machine and Supervisor
- 63 interrupts
- Primary and Secondary delivery modes
- Major interrupt redirection to UCLIC is supported in primary mode
- External interrupt interface implemented (peripheral devices can be connected to the UCLIC via standard riscv-vp interface)
- No H-extension support. The behavior is undefined (and untested) if UIA UCLIC primary mode is enabled and HART is running in VS/VU mode.
- Interrupt lines **52-57** are occupied by platform peripheral devices. Usage of these lines for other purposes must be avoided, to prevent unexpected interrupts.

#### Standard Registers (APLIC-compatible):

- `DOMAINCFG`
- `SOURCECFG` *(includes enable, pending, priority fields)*
- `SETIPNUM`
- `CLRIPNUM`
- `SETIENUM`
- `CLRIENUM`
- `IDELIVERY` *(includes primary mode field)*
- `ITHRESHOLD` *(includes previous threshold field)*
- `TOPI`
- `CLAIMI` *(supports both claim and complete operations in primary mode)*

The above registers have standard AIA APLIC offsets within their domain.

#### Primary mode enable semantics

- Primary mode can be enabled only in M-domain `IDELIVERY`
- Enabling primary mode affects both M and S domains
- The primary mode field in S-domain `IDELIVERY` is read-only 0


#### Additional / Non-standard IDC Registers:

`IDC_DEBUG_HW_THRESHOLD`

Value:
  - Read-only current HW threshold level
  - If no level is set, returns **129**

`IDC_ONLY_COMPLETE`

Value & behavior:
  - Read-only `0`
  - **Primary mode:** read performs complete operation
  - **Secondary mode:** read has no side effect

`IDC_ONLY_CLAIM`

Value & behavior:
  - Read-only value equal to `TOPI`
  - Unlike `CLAIMI`, no complete operation is performed when value is `0`

#### UIA UCLIC memory map

```c
// M-domain UCLIC registers
APLIC_M_DOMAIN_BASE          0x40000000
APLIC_M_IDC_BASE             0x40004000
// M-domain non-standard IDC registers
IDC_DEBUG_HW_THRESHOLD       0x40004020
IDC_ONLY_COMPLETE            0x40004010
IDC_ONLY_CLAIM               0x40004014

// S-domain UCLIC registers
APLIC_S_DOMAIN_BASE          0x40010000
APLIC_S_IDC_BASE             0x40014000
// S-domain non-standard IDC registers
IDC_DEBUG_HW_THRESHOLD       0x40014020
IDC_ONLY_COMPLETE            0x40014010
IDC_ONLY_CLAIM               0x40014014
```

### RTIA (compatible to current v1.0.2 RTIA TRM)

#### Features and configuration

- IMSIC
  - M, S/HS and `7` guests IMSIC files
  - `255` interrupts in each IMSIC file
- AIA HART functionality - major interrupt priorities, major interrupt injection, etc.
- RTIA nested vectored interrupt mode with major interrupt redirection to IMSIC

__NOTE__: in this combined configuration (RTIA + UIA UCLIC) there is no APLIC controller which is capable of sending MSI to the IMSIC. The full-scale APLIC was replaced with a UIA UCLIC.

#### RTIA memory map

```c
IMSIC_M_HART0_SETEIPNUM_LE      0x31000000
IMSIC_S_HART0_SETEIPNUM_LE      0x31001000
```

## Interrupt / trap handling features

### NMIs / RNMIs

- NMIs are not supported, Smrnmi extension is not implemented

### Double trap extension

Double trap extension is enabled by default. It can be disabled by `--double-trap=false` command line option.

Implemented functionality:
- `mstatush.mdt` and `sstatus.sdt` CSRs bits
- `mdt` & `sdt` logic for trap jump and trap return instructions (mret, sret)
- Double Trap exception generation with `mtval`, `mtval2`, `mtinst` CSRs set according to spec

Limitations:
- The M-mode double trap behavior **is incorrect**. We should HALT on double trap in M-mode (as there no Smrnmi extension), but currently we just generate the double trap exception and set the CSRs.
- Double Trap for VS mode is not implemented (no `vstatush.sdt` and corresponding logic)
- `menvcfg.DTE` bit is not implemented (Double Trap always enabled for S-mode)

### WFI instruction

WFI instruction works correctly with UIA UCLIC in both primary and secondary modes. It was tested by usage in Zephyr. WFI should also work in RTIA configuration, but it is not fully tested (especially the cases of pending major interrupts which were injected).

### Stack pointer swap

- RTIA-compliant vertical stack pointer swap is implemented
- Horizontal stack pointer swap (experimental) still presents and it is controlled via `mstatus` and `sstatus` CSRs bits for M and S modes. It does not affect behavior unless explicitly enabled.

## CLINT devices

- M, S and VS timers (RISC-V privilege spec compliant)
- M software interrupt MMIO device (RISC-V privilege spec compliant)
- S software interrupt MMIO device (RTIA compliant)


### CLINT memory map

```c
// S SW interrupt MMIO registers address
SSIP_MMIO               0x0200C000

// M SW interrupt MMIO registers address
MSIP_MMIO               0x02000000

// MTIME (lower 32 bits) register address
MTIME_MMIO              0x0200BFF8
// MTIMEH (upper 32 bits) register address
MTIMEH_MMIO             0x0200BFFC

// MTIMECMP (lower 32 bits) register address
MTIMECMP_MMIO           0x02004000
// MTIMECMPH (upper 32 bits) register address
MTIMECMPH_MMIO          0x02004004
```

## Other platform details

- RAM starting at `0x00000000`, size - 32 MiB
