This file contains list of the changes which affect simulator behavior

Last changes prior riscv-vp-v0.1.0

 - align SSIP MMIO register mapping with the snps spec
 - allow up to 2047 EIIDs
 - Fix mtsp bit position in menvcfg reg (according to 0.8 snps spec version)

Changes since riscv-vp-v0.1.0
 - align m & S iprio arrays with AIA spec for direct & vectored modes
 - set xtopi.iprio field according to AIA spec
 - make mip & sip read-only
 - add SMPU extension according to "Synopsys ARC-V S-mode Memory
    Protection Unit (SMPU)" specification.
 - Add mvien, mvip
 - Add M-> S irq injection
 - fix irqs routing from CLINT to IMSIC in NV mode to avoid EIP refresh where it shouldn't occur
 - set *topei priority field
 - allow mret is only in M mode

Changes since riscv-vp-v0.2.0
