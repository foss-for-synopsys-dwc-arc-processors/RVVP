#pragma once

#include <assert.h>
#include <stdint.h>

#include <stdexcept>
#include <unordered_map>
#include <stack>

#include "config.h"
#include "irq-helpers.h"
#include "irq-prio.h"

#include "trap-codes.h"
#include "util/common.h"

#include "core/common/protected_access.h"

#define BIT(n)  (1ULL << (n))
#define BITS_PER_UNSIGNED_LONG	(sizeof(unsigned long) * 8)
#define GENMASK(h, l)	(((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_UNSIGNED_LONG - 1 - (h))))

namespace rv32 {

constexpr unsigned FS_OFF = 0b00;
constexpr unsigned FS_INITIAL = 0b01;
constexpr unsigned FS_CLEAN = 0b10;
constexpr unsigned FS_DIRTY = 0b11;

inline bool is_valid_privilege_level(PrivilegeLevel mode) {
	return mode == MachineMode || mode == SupervisorMode || mode == UserMode;
}

namespace csr {
template <typename T>
inline bool is_bitset(T &csr, unsigned bitpos) {
	return csr.reg & (1 << bitpos);
}

constexpr uint32_t CSR_TYPE_U = 0b00;
constexpr uint32_t CSR_TYPE_S = 0b01;
constexpr uint32_t CSR_TYPE_HS_VS = 0b10;
constexpr uint32_t CSR_TYPE_M = 0b11;

constexpr uint32_t CSR_TYPE_MASK = 0x300;
constexpr uint32_t CSR_TYPE_SHIFT = 8;


constexpr uint32_t MEDELEG_MASK = 0b1011111111111111; //TODO: is VS ecall (11 bit) delegatable?

constexpr uint32_t MCOUNTEREN_MASK = 0b111;
constexpr uint32_t MCOUNTINHIBIT_MASK = 0b101;


// constexpr uint32_t MSTATUS_MASK = 0b10000000011111111111100110111011;
constexpr uint32_t MSTATUS_MASK = 0b10000000011111111111111111101010;
// constexpr uint32_t SSTATUS_MASK = 0b10000000000011011110000100110011;
constexpr uint32_t SSTATUS_MASK = 0b10000000000011011110011101100010;

constexpr uint32_t MSTATUSH_MASK = 0b00000000000000000000000011110000;

constexpr uint32_t SATP_MASK = 0b10000000001111111111111111111111;
constexpr uint32_t SATP_MODE = 0b10000000000000000000000000000000;

constexpr uint32_t FCSR_MASK = 0b11111111;

// 64 bit timer csrs
constexpr unsigned CYCLE_ADDR = 0xC00;
constexpr unsigned CYCLEH_ADDR = 0xC80;
constexpr unsigned TIME_ADDR = 0xC01;
constexpr unsigned TIMEH_ADDR = 0xC81;
constexpr unsigned INSTRET_ADDR = 0xC02;
constexpr unsigned INSTRETH_ADDR = 0xC82;

// shadows for the above CSRs
constexpr unsigned MCYCLE_ADDR = 0xB00;
constexpr unsigned MCYCLEH_ADDR = 0xB80;
constexpr unsigned MINSTRET_ADDR = 0xB02;
constexpr unsigned MINSTRETH_ADDR = 0xB82;

// debug CSRs
constexpr unsigned TSELECT_ADDR = 0x7A0;
constexpr unsigned TDATA1_ADDR = 0x7A1;
constexpr unsigned TDATA2_ADDR = 0x7A2;
constexpr unsigned TDATA3_ADDR = 0x7A3;
constexpr unsigned DCSR_ADDR = 0x7B0;
constexpr unsigned DPC_ADDR = 0x7B1;
constexpr unsigned DSCRATCH0_ADDR = 0x7B2;
constexpr unsigned DSCRATCH1_ADDR = 0x7B3;

// 32 bit machine CSRs
constexpr unsigned MVENDORID_ADDR = 0xF11;
constexpr unsigned MARCHID_ADDR = 0xF12;
constexpr unsigned MIMPID_ADDR = 0xF13;
constexpr unsigned MHARTID_ADDR = 0xF14;

constexpr unsigned MSTATUS_ADDR = 0x300;
constexpr unsigned MSTATUSH_ADDR = 0x310;
constexpr unsigned MISA_ADDR = 0x301;
constexpr unsigned MEDELEG_ADDR = 0x302;

constexpr unsigned MTVEC_ADDR = 0x305;
constexpr unsigned MCOUNTEREN_ADDR = 0x306;
constexpr unsigned MCOUNTINHIBIT_ADDR = 0x320;

constexpr unsigned MSCRATCH_ADDR = 0x340;
constexpr unsigned MEPC_ADDR = 0x341;
constexpr unsigned MCAUSE_ADDR = 0x342;
constexpr unsigned MTVAL_ADDR = 0x343;

constexpr unsigned MISELECT_ADDR = 0x350;
constexpr unsigned MIREG_ADDR = 0x351;
constexpr unsigned MIREG2_ADDR = 0x352;
constexpr unsigned MIREG3_ADDR = 0x353;
constexpr unsigned MIREG4_ADDR = 0x355;
constexpr unsigned MIREG5_ADDR = 0x356;
constexpr unsigned MIREG6_ADDR = 0x357;

constexpr unsigned MTOPI_ADDR = 0xFB0;
constexpr unsigned MTOPEI_ADDR = 0x35C;

constexpr unsigned MTSP_ADDR = 0x7FF;
constexpr unsigned MTINST_ADDR = 0x34A;
constexpr unsigned MTVAL2_ADDR = 0x34B;

constexpr unsigned PMPCFG0_ADDR = 0x3A0;
constexpr unsigned PMPCFG1_ADDR = 0x3A1;
constexpr unsigned PMPCFG2_ADDR = 0x3A2;
constexpr unsigned PMPCFG3_ADDR = 0x3A3;

constexpr unsigned PMPADDR0_ADDR = 0x3B0;
constexpr unsigned PMPADDR1_ADDR = 0x3B1;
constexpr unsigned PMPADDR2_ADDR = 0x3B2;
constexpr unsigned PMPADDR3_ADDR = 0x3B3;
constexpr unsigned PMPADDR4_ADDR = 0x3B4;
constexpr unsigned PMPADDR5_ADDR = 0x3B5;
constexpr unsigned PMPADDR6_ADDR = 0x3B6;
constexpr unsigned PMPADDR7_ADDR = 0x3B7;
constexpr unsigned PMPADDR8_ADDR = 0x3B8;
constexpr unsigned PMPADDR9_ADDR = 0x3B9;
constexpr unsigned PMPADDR10_ADDR = 0x3BA;
constexpr unsigned PMPADDR11_ADDR = 0x3BB;
constexpr unsigned PMPADDR12_ADDR = 0x3BC;
constexpr unsigned PMPADDR13_ADDR = 0x3BD;
constexpr unsigned PMPADDR14_ADDR = 0x3BE;
constexpr unsigned PMPADDR15_ADDR = 0x3BF;

// 32 bit supervisor CSRs
constexpr unsigned SSTATUS_ADDR = 0x100;
constexpr unsigned STVEC_ADDR = 0x105;
constexpr unsigned SCOUNTEREN_ADDR = 0x106;
constexpr unsigned SSCRATCH_ADDR = 0x140;
constexpr unsigned SEPC_ADDR = 0x141;
constexpr unsigned SCAUSE_ADDR = 0x142;
constexpr unsigned STVAL_ADDR = 0x143;
constexpr unsigned SATP_ADDR = 0x180;

constexpr unsigned SISELECT_ADDR = 0x150;
constexpr uint32_t SISELECT_MASK = 0xFFF;
constexpr unsigned SIREG_ADDR = 0x151;
constexpr unsigned SIREG2_ADDR = 0x152;
constexpr unsigned SIREG3_ADDR = 0x153;
constexpr unsigned SIREG4_ADDR = 0x155;
constexpr unsigned SIREG5_ADDR = 0x156;
constexpr unsigned SIREG6_ADDR = 0x157;

constexpr unsigned STOPI_ADDR = 0xDB0;
constexpr unsigned STOPEI_ADDR = 0x15C;

constexpr unsigned STSP_ADDR = 0x5FF;

// 32 bit H-extended supervisor CSRs
constexpr uint32_t HSTATUS_MASK = 0b00000000011101111111001111100000;
constexpr unsigned HSTATUS_ADDR = 0x600;
constexpr uint32_t HEDELEG_MASK = 0b111000111111111;
constexpr unsigned HEDELEG_ADDR = 0x602;

constexpr unsigned HCONTEXT_ADDR = 0x6A8;
constexpr unsigned HVIP_ADDR = 0x645;
constexpr unsigned HTSP_ADDR = 0xAFF;
constexpr unsigned HGATP_ADDR = 0x680;
constexpr unsigned HMPUMASK_ADDR = 0x620;
constexpr unsigned HTVAL_ADDR = 0x643;
constexpr unsigned HTINST_ADDR = 0x64A;

// 32 bit VS CSRs
constexpr uint32_t VSSTATUS_MASK = SSTATUS_MASK;
constexpr unsigned VSSTATUS_ADDR = 0x200;

constexpr unsigned VSTVEC_ADDR = 0x205;
constexpr unsigned VSSCRATCH_ADDR = 0x240;
constexpr unsigned VSEPC_ADDR = 0x241;
constexpr unsigned VSCAUSE_ADDR = 0x242;
constexpr unsigned VSTVAL_ADDR = 0x243;
constexpr unsigned VSTSP_ADDR = 0x6FF;

constexpr unsigned VSISELECT_ADDR = 0x250;
constexpr uint32_t VSISELECT_MASK = 0xFFF;
constexpr unsigned VSIREG_ADDR = 0x251;
constexpr unsigned VSIREG2_ADDR = 0x252;
constexpr unsigned VSIREG3_ADDR = 0x253;
constexpr unsigned VSIREG4_ADDR = 0x255;
constexpr unsigned VSIREG5_ADDR = 0x256;
constexpr unsigned VSIREG6_ADDR = 0x257;

constexpr unsigned VSTOPI_ADDR = 0xEB0;
constexpr unsigned VSTOPEI_ADDR = 0x25C;

constexpr unsigned VSMPUMASK_ADDR = 0x228;
constexpr unsigned VSATP_ADDR = 0x280;

// floating point CSRs
constexpr unsigned FFLAGS_ADDR = 0x001;
constexpr unsigned FRM_ADDR = 0x002;
constexpr unsigned FCSR_ADDR = 0x003;

// performance counters
constexpr unsigned HPMCOUNTER3_ADDR = 0xC03;
constexpr unsigned HPMCOUNTER4_ADDR = 0xC04;
constexpr unsigned HPMCOUNTER5_ADDR = 0xC05;
constexpr unsigned HPMCOUNTER6_ADDR = 0xC06;
constexpr unsigned HPMCOUNTER7_ADDR = 0xC07;
constexpr unsigned HPMCOUNTER8_ADDR = 0xC08;
constexpr unsigned HPMCOUNTER9_ADDR = 0xC09;
constexpr unsigned HPMCOUNTER10_ADDR = 0xC0A;
constexpr unsigned HPMCOUNTER11_ADDR = 0xC0B;
constexpr unsigned HPMCOUNTER12_ADDR = 0xC0C;
constexpr unsigned HPMCOUNTER13_ADDR = 0xC0D;
constexpr unsigned HPMCOUNTER14_ADDR = 0xC0E;
constexpr unsigned HPMCOUNTER15_ADDR = 0xC0F;
constexpr unsigned HPMCOUNTER16_ADDR = 0xC10;
constexpr unsigned HPMCOUNTER17_ADDR = 0xC11;
constexpr unsigned HPMCOUNTER18_ADDR = 0xC12;
constexpr unsigned HPMCOUNTER19_ADDR = 0xC13;
constexpr unsigned HPMCOUNTER20_ADDR = 0xC14;
constexpr unsigned HPMCOUNTER21_ADDR = 0xC15;
constexpr unsigned HPMCOUNTER22_ADDR = 0xC16;
constexpr unsigned HPMCOUNTER23_ADDR = 0xC17;
constexpr unsigned HPMCOUNTER24_ADDR = 0xC18;
constexpr unsigned HPMCOUNTER25_ADDR = 0xC19;
constexpr unsigned HPMCOUNTER26_ADDR = 0xC1A;
constexpr unsigned HPMCOUNTER27_ADDR = 0xC1B;
constexpr unsigned HPMCOUNTER28_ADDR = 0xC1C;
constexpr unsigned HPMCOUNTER29_ADDR = 0xC1D;
constexpr unsigned HPMCOUNTER30_ADDR = 0xC1E;
constexpr unsigned HPMCOUNTER31_ADDR = 0xC1F;

constexpr unsigned HPMCOUNTER3H_ADDR = 0xC83;
constexpr unsigned HPMCOUNTER4H_ADDR = 0xC84;
constexpr unsigned HPMCOUNTER5H_ADDR = 0xC85;
constexpr unsigned HPMCOUNTER6H_ADDR = 0xC86;
constexpr unsigned HPMCOUNTER7H_ADDR = 0xC87;
constexpr unsigned HPMCOUNTER8H_ADDR = 0xC88;
constexpr unsigned HPMCOUNTER9H_ADDR = 0xC89;
constexpr unsigned HPMCOUNTER10H_ADDR = 0xC8A;
constexpr unsigned HPMCOUNTER11H_ADDR = 0xC8B;
constexpr unsigned HPMCOUNTER12H_ADDR = 0xC8C;
constexpr unsigned HPMCOUNTER13H_ADDR = 0xC8D;
constexpr unsigned HPMCOUNTER14H_ADDR = 0xC8E;
constexpr unsigned HPMCOUNTER15H_ADDR = 0xC8F;
constexpr unsigned HPMCOUNTER16H_ADDR = 0xC90;
constexpr unsigned HPMCOUNTER17H_ADDR = 0xC91;
constexpr unsigned HPMCOUNTER18H_ADDR = 0xC92;
constexpr unsigned HPMCOUNTER19H_ADDR = 0xC93;
constexpr unsigned HPMCOUNTER20H_ADDR = 0xC94;
constexpr unsigned HPMCOUNTER21H_ADDR = 0xC95;
constexpr unsigned HPMCOUNTER22H_ADDR = 0xC96;
constexpr unsigned HPMCOUNTER23H_ADDR = 0xC97;
constexpr unsigned HPMCOUNTER24H_ADDR = 0xC98;
constexpr unsigned HPMCOUNTER25H_ADDR = 0xC99;
constexpr unsigned HPMCOUNTER26H_ADDR = 0xC9A;
constexpr unsigned HPMCOUNTER27H_ADDR = 0xC9B;
constexpr unsigned HPMCOUNTER28H_ADDR = 0xC9C;
constexpr unsigned HPMCOUNTER29H_ADDR = 0xC9D;
constexpr unsigned HPMCOUNTER30H_ADDR = 0xC9E;
constexpr unsigned HPMCOUNTER31H_ADDR = 0xC9F;

constexpr unsigned MHPMCOUNTER3_ADDR = 0xB03;
constexpr unsigned MHPMCOUNTER4_ADDR = 0xB04;
constexpr unsigned MHPMCOUNTER5_ADDR = 0xB05;
constexpr unsigned MHPMCOUNTER6_ADDR = 0xB06;
constexpr unsigned MHPMCOUNTER7_ADDR = 0xB07;
constexpr unsigned MHPMCOUNTER8_ADDR = 0xB08;
constexpr unsigned MHPMCOUNTER9_ADDR = 0xB09;
constexpr unsigned MHPMCOUNTER10_ADDR = 0xB0A;
constexpr unsigned MHPMCOUNTER11_ADDR = 0xB0B;
constexpr unsigned MHPMCOUNTER12_ADDR = 0xB0C;
constexpr unsigned MHPMCOUNTER13_ADDR = 0xB0D;
constexpr unsigned MHPMCOUNTER14_ADDR = 0xB0E;
constexpr unsigned MHPMCOUNTER15_ADDR = 0xB0F;
constexpr unsigned MHPMCOUNTER16_ADDR = 0xB10;
constexpr unsigned MHPMCOUNTER17_ADDR = 0xB11;
constexpr unsigned MHPMCOUNTER18_ADDR = 0xB12;
constexpr unsigned MHPMCOUNTER19_ADDR = 0xB13;
constexpr unsigned MHPMCOUNTER20_ADDR = 0xB14;
constexpr unsigned MHPMCOUNTER21_ADDR = 0xB15;
constexpr unsigned MHPMCOUNTER22_ADDR = 0xB16;
constexpr unsigned MHPMCOUNTER23_ADDR = 0xB17;
constexpr unsigned MHPMCOUNTER24_ADDR = 0xB18;
constexpr unsigned MHPMCOUNTER25_ADDR = 0xB19;
constexpr unsigned MHPMCOUNTER26_ADDR = 0xB1A;
constexpr unsigned MHPMCOUNTER27_ADDR = 0xB1B;
constexpr unsigned MHPMCOUNTER28_ADDR = 0xB1C;
constexpr unsigned MHPMCOUNTER29_ADDR = 0xB1D;
constexpr unsigned MHPMCOUNTER30_ADDR = 0xB1E;
constexpr unsigned MHPMCOUNTER31_ADDR = 0xB1F;

constexpr unsigned MHPMCOUNTER3H_ADDR = 0xB83;
constexpr unsigned MHPMCOUNTER4H_ADDR = 0xB84;
constexpr unsigned MHPMCOUNTER5H_ADDR = 0xB85;
constexpr unsigned MHPMCOUNTER6H_ADDR = 0xB86;
constexpr unsigned MHPMCOUNTER7H_ADDR = 0xB87;
constexpr unsigned MHPMCOUNTER8H_ADDR = 0xB88;
constexpr unsigned MHPMCOUNTER9H_ADDR = 0xB89;
constexpr unsigned MHPMCOUNTER10H_ADDR = 0xB8A;
constexpr unsigned MHPMCOUNTER11H_ADDR = 0xB8B;
constexpr unsigned MHPMCOUNTER12H_ADDR = 0xB8C;
constexpr unsigned MHPMCOUNTER13H_ADDR = 0xB8D;
constexpr unsigned MHPMCOUNTER14H_ADDR = 0xB8E;
constexpr unsigned MHPMCOUNTER15H_ADDR = 0xB8F;
constexpr unsigned MHPMCOUNTER16H_ADDR = 0xB90;
constexpr unsigned MHPMCOUNTER17H_ADDR = 0xB91;
constexpr unsigned MHPMCOUNTER18H_ADDR = 0xB92;
constexpr unsigned MHPMCOUNTER19H_ADDR = 0xB93;
constexpr unsigned MHPMCOUNTER20H_ADDR = 0xB94;
constexpr unsigned MHPMCOUNTER21H_ADDR = 0xB95;
constexpr unsigned MHPMCOUNTER22H_ADDR = 0xB96;
constexpr unsigned MHPMCOUNTER23H_ADDR = 0xB97;
constexpr unsigned MHPMCOUNTER24H_ADDR = 0xB98;
constexpr unsigned MHPMCOUNTER25H_ADDR = 0xB99;
constexpr unsigned MHPMCOUNTER26H_ADDR = 0xB9A;
constexpr unsigned MHPMCOUNTER27H_ADDR = 0xB9B;
constexpr unsigned MHPMCOUNTER28H_ADDR = 0xB9C;
constexpr unsigned MHPMCOUNTER29H_ADDR = 0xB9D;
constexpr unsigned MHPMCOUNTER30H_ADDR = 0xB9E;
constexpr unsigned MHPMCOUNTER31H_ADDR = 0xB9F;

constexpr unsigned MHPMEVENT3_ADDR = 0x323;
constexpr unsigned MHPMEVENT4_ADDR = 0x324;
constexpr unsigned MHPMEVENT5_ADDR = 0x325;
constexpr unsigned MHPMEVENT6_ADDR = 0x326;
constexpr unsigned MHPMEVENT7_ADDR = 0x327;
constexpr unsigned MHPMEVENT8_ADDR = 0x328;
constexpr unsigned MHPMEVENT9_ADDR = 0x329;
constexpr unsigned MHPMEVENT10_ADDR = 0x32A;
constexpr unsigned MHPMEVENT11_ADDR = 0x32B;
constexpr unsigned MHPMEVENT12_ADDR = 0x32C;
constexpr unsigned MHPMEVENT13_ADDR = 0x32D;
constexpr unsigned MHPMEVENT14_ADDR = 0x32E;
constexpr unsigned MHPMEVENT15_ADDR = 0x32F;
constexpr unsigned MHPMEVENT16_ADDR = 0x330;
constexpr unsigned MHPMEVENT17_ADDR = 0x331;
constexpr unsigned MHPMEVENT18_ADDR = 0x332;
constexpr unsigned MHPMEVENT19_ADDR = 0x333;
constexpr unsigned MHPMEVENT20_ADDR = 0x334;
constexpr unsigned MHPMEVENT21_ADDR = 0x335;
constexpr unsigned MHPMEVENT22_ADDR = 0x336;
constexpr unsigned MHPMEVENT23_ADDR = 0x337;
constexpr unsigned MHPMEVENT24_ADDR = 0x338;
constexpr unsigned MHPMEVENT25_ADDR = 0x339;
constexpr unsigned MHPMEVENT26_ADDR = 0x33A;
constexpr unsigned MHPMEVENT27_ADDR = 0x33B;
constexpr unsigned MHPMEVENT28_ADDR = 0x33C;
constexpr unsigned MHPMEVENT29_ADDR = 0x33D;
constexpr unsigned MHPMEVENT30_ADDR = 0x33E;
constexpr unsigned MHPMEVENT31_ADDR = 0x33F;

// ! SPMP CSRs should be allocated contiguously
constexpr unsigned SPMPCFG0_ADDR = 0x1A0;
constexpr unsigned SPMPCFG1_ADDR = 0x1A1;
constexpr unsigned SPMPCFG2_ADDR = 0x1A2;
constexpr unsigned SPMPCFG3_ADDR = 0x1A3;
constexpr unsigned SPMPCFG4_ADDR = 0x1A4;
constexpr unsigned SPMPCFG5_ADDR = 0x1A5;
constexpr unsigned SPMPCFG6_ADDR = 0x1A6;
constexpr unsigned SPMPCFG7_ADDR = 0x1A7;
constexpr unsigned SPMPCFG8_ADDR = 0x1A8;
constexpr unsigned SPMPCFG9_ADDR = 0x1A9;
constexpr unsigned SPMPCFG10_ADDR = 0x1AA;
constexpr unsigned SPMPCFG11_ADDR = 0x1AB;
constexpr unsigned SPMPCFG12_ADDR = 0x1AC;
constexpr unsigned SPMPCFG13_ADDR = 0x1AD;
constexpr unsigned SPMPCFG14_ADDR = 0x1AE;
constexpr unsigned SPMPCFG15_ADDR = 0x1AF;

// ! SPMP CSRs should be allocated contiguously
constexpr unsigned SPMPADDR0_ADDR = 0x1B0;
constexpr unsigned SPMPADDR1_ADDR = 0x1B1;
constexpr unsigned SPMPADDR2_ADDR = 0x1B2;
constexpr unsigned SPMPADDR3_ADDR = 0x1B3;
constexpr unsigned SPMPADDR4_ADDR = 0x1B4;
constexpr unsigned SPMPADDR5_ADDR = 0x1B5;
constexpr unsigned SPMPADDR6_ADDR = 0x1B6;
constexpr unsigned SPMPADDR7_ADDR = 0x1B7;
constexpr unsigned SPMPADDR8_ADDR = 0x1B8;
constexpr unsigned SPMPADDR9_ADDR = 0x1B9;
constexpr unsigned SPMPADDR10_ADDR = 0x1BA;
constexpr unsigned SPMPADDR11_ADDR = 0x1BB;
constexpr unsigned SPMPADDR12_ADDR = 0x1BC;
constexpr unsigned SPMPADDR13_ADDR = 0x1BD;
constexpr unsigned SPMPADDR14_ADDR = 0x1BE;
constexpr unsigned SPMPADDR15_ADDR = 0x1BF;
constexpr unsigned SPMPADDR16_ADDR = 0x1C0;
constexpr unsigned SPMPADDR17_ADDR = 0x1C1;
constexpr unsigned SPMPADDR18_ADDR = 0x1C2;
constexpr unsigned SPMPADDR19_ADDR = 0x1C3;
constexpr unsigned SPMPADDR20_ADDR = 0x1C4;
constexpr unsigned SPMPADDR21_ADDR = 0x1C5;
constexpr unsigned SPMPADDR22_ADDR = 0x1C6;
constexpr unsigned SPMPADDR23_ADDR = 0x1C7;
constexpr unsigned SPMPADDR24_ADDR = 0x1C8;
constexpr unsigned SPMPADDR25_ADDR = 0x1C9;
constexpr unsigned SPMPADDR26_ADDR = 0x1CA;
constexpr unsigned SPMPADDR27_ADDR = 0x1CB;
constexpr unsigned SPMPADDR28_ADDR = 0x1CC;
constexpr unsigned SPMPADDR29_ADDR = 0x1CD;
constexpr unsigned SPMPADDR30_ADDR = 0x1CE;
constexpr unsigned SPMPADDR31_ADDR = 0x1CF;
constexpr unsigned SPMPADDR32_ADDR = 0x1D0;
constexpr unsigned SPMPADDR33_ADDR = 0x1D1;
constexpr unsigned SPMPADDR34_ADDR = 0x1D2;
constexpr unsigned SPMPADDR35_ADDR = 0x1D3;
constexpr unsigned SPMPADDR36_ADDR = 0x1D4;
constexpr unsigned SPMPADDR37_ADDR = 0x1D5;
constexpr unsigned SPMPADDR38_ADDR = 0x1D6;
constexpr unsigned SPMPADDR39_ADDR = 0x1D7;
constexpr unsigned SPMPADDR40_ADDR = 0x1D8;
constexpr unsigned SPMPADDR41_ADDR = 0x1D9;
constexpr unsigned SPMPADDR42_ADDR = 0x1DA;
constexpr unsigned SPMPADDR43_ADDR = 0x1DB;
constexpr unsigned SPMPADDR44_ADDR = 0x1DC;
constexpr unsigned SPMPADDR45_ADDR = 0x1DD;
constexpr unsigned SPMPADDR46_ADDR = 0x1DE;
constexpr unsigned SPMPADDR47_ADDR = 0x1DF;
constexpr unsigned SPMPADDR48_ADDR = 0x1E0;
constexpr unsigned SPMPADDR49_ADDR = 0x1E1;
constexpr unsigned SPMPADDR50_ADDR = 0x1E2;
constexpr unsigned SPMPADDR51_ADDR = 0x1E3;
constexpr unsigned SPMPADDR52_ADDR = 0x1E4;
constexpr unsigned SPMPADDR53_ADDR = 0x1E5;
constexpr unsigned SPMPADDR54_ADDR = 0x1E6;
constexpr unsigned SPMPADDR55_ADDR = 0x1E7;
constexpr unsigned SPMPADDR56_ADDR = 0x1E8;
constexpr unsigned SPMPADDR57_ADDR = 0x1E9;
constexpr unsigned SPMPADDR58_ADDR = 0x1EA;
constexpr unsigned SPMPADDR59_ADDR = 0x1EB;
constexpr unsigned SPMPADDR60_ADDR = 0x1EC;
constexpr unsigned SPMPADDR61_ADDR = 0x1ED;
constexpr unsigned SPMPADDR62_ADDR = 0x1EE;
constexpr unsigned SPMPADDR63_ADDR = 0x1EF;

// ! SPMP CSRs should be allocated contiguously
constexpr unsigned SPMPSWITCH0_ADDR = 0x170;
constexpr unsigned SPMPSWITCH1_ADDR = 0x171;

// SMPU extension
constexpr unsigned SMPUMASK_ADDR = 0x128;

inline unsigned xireg_to_xselect_offset(unsigned xireg) {
	switch (xireg)
	{
	case MIREG_ADDR:
	case SIREG_ADDR:
	case VSIREG_ADDR:
		return 0;
	case MIREG2_ADDR:
	case SIREG2_ADDR:
	case VSIREG2_ADDR:
		return 1;
	case MIREG3_ADDR:
	case SIREG3_ADDR:
	case VSIREG3_ADDR:
		return 2;
	case MIREG4_ADDR:
	case SIREG4_ADDR:
	case VSIREG4_ADDR:
		return 3;
	case MIREG5_ADDR:
	case SIREG5_ADDR:
	case VSIREG5_ADDR:
		return 4;
	case MIREG6_ADDR:
	case SIREG6_ADDR:
	case VSIREG6_ADDR:
		return 5;

	default:
		return 0;
	}
}

}  // namespace csr

// vgein - [1 : iss_config::MAX_GUEST]
static uint32_t vgein_to_id(uint32_t vgein) {
	uint32_t id = vgein - 1;

	if (id > (iss_config::MAX_GUEST - 1)) {
		id = iss_config::MAX_GUEST - 1;
	}

	return id;
}

// id - [0 : iss_config::MAX_GUEST - 1]
static uint32_t id_to_vgein(uint32_t id) {
	uint32_t vgein = id + 1;

	if (vgein == 0 || vgein > iss_config::MAX_GUEST) {
		vgein = iss_config::MAX_GUEST;
	}

	return vgein;
}

struct csr_if {
	virtual void checked_write(uint32_t val) = 0;
	virtual uint32_t checked_read(void) = 0;

	virtual ~csr_if() {};
};

struct csr_32 {
	uint32_t reg = 0;
};

struct csr_misa {
	csr_misa() {
		init();
	}

	union {
		uint32_t reg = 0;
		struct {
			unsigned extensions : 26;
			unsigned wiri : 4;
			unsigned mxl : 2;
		} fields;
	};

	bool has_C_extension() {
		return fields.extensions & C;
	}

	bool has_E_base_isa() {
		return fields.extensions & E;
	}

	void select_E_base_isa() {
		fields.extensions &= ~I;
		fields.extensions |= E;
	}

	bool has_user_mode_extension() {
		return true;
	}

	bool has_supervisor_mode_extension() {
		return fields.extensions & S;
	}

	bool has_hypervisor_mode_extension() {
		return fields.extensions & H;
	}

	enum {
		A = 1,
		C = 1 << 2,
		D = 1 << 3,
		E = 1 << 4,
		F = 1 << 5,
		H = 1 << 7,  // Hypervisor extension
		I = 1 << 8,
		M = 1 << 12,
		N = 1 << 13,
		S = 1 << 18,
	};

	void init() {
		fields.extensions = I | M | A | F | C | N | S | H;  // IMACF + NUSH  // TODO: drop N (User-Level Interrupts extension)
		fields.wiri = 0;
		fields.mxl = 1;  // RV32
	}
};

constexpr unsigned M_ISA_EXT = csr_misa::M;
constexpr unsigned A_ISA_EXT = csr_misa::A;
constexpr unsigned F_ISA_EXT = csr_misa::F;
constexpr unsigned D_ISA_EXT = csr_misa::D;
constexpr unsigned C_ISA_EXT = csr_misa::C;
constexpr unsigned H_ISA_EXT = csr_misa::H;

struct csr_mvendorid {
	union {
		uint32_t reg = 0;
		struct {
			unsigned offset : 7;
			unsigned bank : 25;
		} fields;
	};
};

struct csr_mstatus {
	union {
		uint32_t reg = 0;
		struct {
			unsigned wpri1 : 1;
			unsigned sie : 1;
			unsigned wpri2 : 1;
			unsigned mie : 1;
			unsigned wpri3 : 1;
			unsigned spie : 1;
			unsigned ube : 1;
			unsigned mpie : 1;
			unsigned spp : 1;
			unsigned vs : 2;
			unsigned mpp : 2;
			unsigned fs : 2;
			unsigned xs : 2;
			unsigned mprv : 1;
			unsigned sum : 1;
			unsigned mxr : 1;
			unsigned tvm : 1;
			unsigned tw : 1;
			unsigned tsr : 1;
			unsigned wpri5 : 8;
			unsigned sd : 1;
		} fields;
	};
};

struct csr_vsstatus {
	union {
		uint32_t reg = 0;
		struct {
			unsigned wpri1 : 1;
			unsigned sie : 1;
			unsigned wpri2 : 3;
			unsigned spie : 1;
			unsigned ube : 1;
			unsigned wpri3 : 1;
			unsigned spp : 1;
			unsigned vs : 2;
			unsigned wpri4 : 2;
			unsigned fs : 2;
			unsigned xs : 2;
			unsigned wpri5 : 1;
			unsigned sum : 1;
			unsigned mxr : 1;
			unsigned wpri6 : 11;
			unsigned sd : 1;
		} fields;
	};
};

struct csr_mstatush {
	union {
		uint32_t reg = 0;
		struct {
			unsigned wpri1 : 4;
			unsigned sbe : 1;
			unsigned mbe : 1;
			unsigned gva : 1;
			unsigned mpv : 1;
			unsigned wpri2 : 24;
		} fields;
	};
};

struct csr_hstatus {
	union {
		uint32_t reg = 0;
		struct {
			unsigned wpri1 : 5;
			unsigned vsbe : 1;
			unsigned gva : 1;
			unsigned spv : 1;
			unsigned spvp : 1;
			unsigned hu : 1;
			unsigned wpri2 : 2;
			unsigned vgein : 6;
			unsigned vsum : 1;
			unsigned wpri3 : 1;
			unsigned vtvm : 1;
			unsigned vtw : 1;
			unsigned vtsr : 1;
			unsigned wpri4 : 9;
		} fields;
	};

	void checked_write(uint32_t value) {
		reg = (reg & ~HSTATUS_MASK) | (value & HSTATUS_MASK);

		if (fields.vgein >= iss_config::MAX_GUEST) {
			fields.vgein = iss_config::MAX_GUEST;
		}
	}

	unsigned get_guest_id(void) {
		return vgein_to_id(fields.vgein);
	}

	unsigned get_vgein(void) {
		return fields.vgein;
	}

	bool is_imsic_connected(void) {
		return fields.vgein != 0;
	}

	static constexpr uint32_t VGEIN_FIRST_GUEST = 1;
	// One for each guest and one for VGEIN = 0
	static constexpr uint32_t MAX_VGEIN_BANKS = iss_config::MAX_GUEST + VGEIN_FIRST_GUEST;

private:
	static constexpr uint32_t HSTATUS_MASK = 0b00000000011101111111001111100000;
};

struct csr_mtvec {
	union {
		uint32_t reg = 0;
		struct {
			unsigned mode : 2;   // WARL
			unsigned base : 30;  // WARL
		} fields;
	};

	uint32_t get_base_address() {
		return fields.base << 2;
	}

	enum Mode { Direct = 0, Vectored = 1, SnpsNestedVectored = 3 };

	void checked_write(uint32_t val) {
		reg = val;

		// As per doc: setting bit#1 enforces setting bit#0
		if (fields.mode != Direct && fields.mode != Vectored && fields.mode != SnpsNestedVectored)
			fields.mode = SnpsNestedVectored;
	}
};

struct csr64_bit_ops {
	static void write_lo(uint64_t & reg, uint64_t mask, uint32_t val) {
		write_64(reg, mask & WRITE_LO_MASK, lo_to_reg(val));
	};

	static void write_hi(uint64_t & reg, uint64_t mask, uint32_t val) {
		write_64(reg, mask & WRITE_HI_MASK, hi_to_reg(val));
	};

	static void write_64(uint64_t & reg, uint64_t mask, uint64_t value) {
		reg = (reg & ~mask) | (value & mask);
	};

	static uint32_t read_lo(uint64_t reg, uint64_t mask) {
		return reg_to_lo(read_64(reg, mask));
	};

	static uint32_t read_hi(uint64_t reg, uint64_t mask) {
		return reg_to_hi(read_64(reg, mask));
	};

	static uint64_t read_64(uint64_t reg, uint64_t mask) {
		return reg & mask;
	};

	static uint32_t reg_to_lo(uint64_t val) {
		return (uint32_t)val;
	};

	static uint32_t reg_to_hi(uint64_t val) {
		return (uint32_t)(val >> 32);
	};

	static constexpr uint64_t lo_to_reg(uint32_t val) {
		return val;
	};

	static constexpr uint64_t hi_to_reg(uint32_t val) {
		return ((uint64_t)val) << 32;
	};

	static constexpr uint64_t WRITE_HI_MASK = ((uint64_t)UINT32_MAX) << 32;
	static constexpr uint64_t WRITE_LO_MASK = UINT32_MAX;
};

struct csrs_clint_pend {
	// For each writable bit in sie, the corresponding bit shall be read-only zero in both hip and hie.
	static constexpr uint32_t SIE_MASK = BIT(EXC_S_SOFTWARE_INTERRUPT) |
					     BIT(EXC_S_TIMER_INTERRUPT) |
					     BIT(EXC_S_EXTERNAL_INTERRUPT);
	static constexpr uint32_t HIE_MASK = BIT(EXC_S_GUEST_EXTERNAL_INTERRUPT) |
					     BIT(EXC_VS_SOFTWARE_INTERRUPT) |
					     BIT(EXC_VS_TIMER_INTERRUPT) |
					     BIT(EXC_VS_EXTERNAL_INTERRUPT);
	static constexpr uint32_t MIE_MASK = BIT(EXC_M_SOFTWARE_INTERRUPT) |
					     BIT(EXC_M_TIMER_INTERRUPT) |
					     BIT(EXC_M_EXTERNAL_INTERRUPT) |
					     SIE_MASK | HIE_MASK;

	static constexpr uint32_t VSIE_MASK = BIT(EXC_VS_SOFTWARE_INTERRUPT) |
					      BIT(EXC_VS_TIMER_INTERRUPT) |
					      BIT(EXC_VS_EXTERNAL_INTERRUPT);



	static constexpr uint64_t MIx_VS_MASK = BIT(EXC_VS_SOFTWARE_INTERRUPT) |
						BIT(EXC_VS_TIMER_INTERRUPT) |
						BIT(EXC_VS_EXTERNAL_INTERRUPT);
	static constexpr uint64_t VSIx_VS_MASK = BIT(EXC_S_SOFTWARE_INTERRUPT) |
						BIT(EXC_S_TIMER_INTERRUPT) |
						BIT(EXC_S_EXTERNAL_INTERRUPT);

	// first 12 bits
	static constexpr uint64_t MIx_LEVELED_MASK = GENMASK(EXC_M_EXTERNAL_INTERRUPT, 0);
	static constexpr uint64_t MIx_NON_LEVELED_MASK = BIT(EXC_COUNTER_OVREFLOW_INTERRUPT) |
							 BIT(EXC_DEBUG_TRACE_INTERRUPT) |
							 BIT(EXC_LOW_PRIO_RAS_INTERRUPT) |
							 BIT(EXC_HIGH_PRIO_RAS_INTERRUPT) |
							 BIT(EXC_WDT_INTERRUPT);

	// edge irqs - the standard ones
	// TODO: do we treat WDT as edge or level IRQ? NOTE: It's edge here
	static constexpr uint64_t MIx_IRQ_TYPE_EDGE_MASK = MIx_NON_LEVELED_MASK;
	static constexpr uint64_t MIx_IRQ_TYPE_LEVL_MASK = MIE_MASK;

	static constexpr uint32_t MIE_TO_VSIE_SHIFT = 1; // value we need to shift MIE to have VS bits on S bits place

	// legacy masks
	static_assert(MIE_MASK == 0b1111011101110);
	static_assert(SIE_MASK == 0b0001000100010);
	static_assert(HIE_MASK == 0b1010001000100);

	static_assert(HIE_MASK ^ SIE_MASK);
	static_assert((MIE_MASK & SIE_MASK) == SIE_MASK);
	static_assert((MIE_MASK & HIE_MASK) == HIE_MASK);

	struct xie : public csr64_bit_ops {
		csrs_clint_pend & clint;

		xie(struct csrs_clint_pend & clint) : clint(clint) {}

		union {
			uint64_t reg = 0;
			struct {
				unsigned wpri0 : 1;
				unsigned ssie : 1;
				unsigned vssie : 1;
				unsigned msie : 1;

				unsigned wpri1 : 1;
				unsigned stie : 1;
				unsigned vstie : 1;
				unsigned mtie : 1;

				unsigned wpri2 : 1;
				unsigned seie : 1;
				unsigned vseie : 1;
				unsigned meie : 1;

				unsigned sgeie : 1;

				unsigned wpri4 : 19;
				unsigned wiri5 : 32;
			} fields;
		};

		uint64_t sie_routed_read_64(void) {
			auto [delegated, injected] = clint.s_route_masks();

			// Alias of mie[n]
			delegated &= reg;
			// shadow enable
			injected &= clint.mvirt.shadow_enables;

			return read_64(delegated | injected, SIE_READ_MASK);
		}

		void sie_routed_write_64(uint64_t write_mask, uint64_t value) {
			auto [delegated, injected] = clint.s_route_masks();

			write_64(reg, write_mask & SIE_WRITE_MASK & delegated, value);
			write_64(clint.mvirt.shadow_enables, write_mask & SIE_WRITE_MASK & injected, value);
		}

		void checked_write_sie(uint32_t val) {
			sie_routed_write_64(WRITE_LO_MASK, lo_to_reg(val));
		}

		void checked_write_sieh(uint32_t val) {
			sie_routed_write_64(WRITE_HI_MASK, hi_to_reg(val));
		}

		uint64_t vsie_routed_read_64(void) {
			auto [delegated, injected] = clint.vs_route_masks();

			uint64_t sie = ~MIx_LEVELED_MASK & sie_routed_read_64();
			uint64_t hie = MIx_LEVELED_MASK & checked_read_hie();

			// Alias of sie[n] / hie[n]
			delegated = delegated & (sie | hie);
			// shadow enable
			injected = injected & clint.hvirt.shadow_enables;

			return read_64(delegated | injected, VSIE_READ_MASK);
		}

		template <uint64_t write_mask>
		void vsie_routed_write_64(uint64_t value) {
			value = vs_to_m_bits(value);

			auto [delegated, injected] = clint.vs_route_masks();

			uint64_t delegated_mask = write_mask & VSIE_WRITE_MASK & delegated;

			sie_routed_write_64(delegated_mask, value);
			hie_routed_write_32(delegated_mask, value);
			write_64(clint.hvirt.shadow_enables, write_mask & VSIE_WRITE_MASK & injected, value);
		}

		void checked_write_vsie(uint32_t val) {
			vsie_routed_write_64<WRITE_LO_MASK>(lo_to_reg(val));
		}

		void checked_write_vsieh(uint32_t val) {
			vsie_routed_write_64<WRITE_HI_MASK>(hi_to_reg(val));
		}

		/* low 32 bit registers xie */
		void checked_write_mie(uint32_t val) {
			write_lo(reg, MIE_WRITE_MASK, val);
		}

		void hie_routed_write_32(uint64_t write_mask, uint32_t val) {
			write_lo(reg, write_mask & HIE_WRITE_MASK, val);
		}

		void checked_write_hie(uint32_t val) {
			hie_routed_write_32(WRITE_LO_MASK, val);
		}

		uint32_t checked_read_mie(void) {
			return read_lo(reg, MIE_READ_MASK);
		}

		uint32_t checked_read_sie(void) {
			return reg_to_lo(sie_routed_read_64());
		}

		uint32_t checked_read_hie(void) {
			return read_lo(reg, HIE_READ_MASK);
		}

		uint32_t checked_read_vsie(void) {
			return reg_to_lo(m_to_vs_bits(vsie_routed_read_64()));
		}

		/* high 32 bit registers xie */
		void checked_write_mieh(uint32_t val) {
			write_hi(reg, MIE_WRITE_MASK, val);
		}

		uint32_t checked_read_mieh(void) {
			return read_hi(reg, MIE_READ_MASK);
		}

		uint32_t checked_read_sieh(void) {
			return reg_to_hi(sie_routed_read_64());
		}

		uint32_t checked_read_vsieh(void) {
			return reg_to_hi(m_to_vs_bits(vsie_routed_read_64()));
		}

		// xie
		static constexpr uint64_t MIE_WRITE_MASK = MIx_NON_LEVELED_MASK | MIE_MASK;
		static constexpr uint64_t MIE_READ_MASK = MIx_NON_LEVELED_MASK | MIE_MASK;

		static constexpr uint64_t SIE_WRITE_MASK = MIx_NON_LEVELED_MASK | SIE_MASK;
		static constexpr uint64_t SIE_READ_MASK = MIx_NON_LEVELED_MASK | SIE_MASK;

		static constexpr uint64_t HIE_WRITE_MASK = HIE_MASK;
		static constexpr uint64_t HIE_READ_MASK = HIE_MASK;

		static constexpr uint64_t VSIE_WRITE_MASK = MIx_NON_LEVELED_MASK | VSIE_MASK;
		static constexpr uint64_t VSIE_READ_MASK = MIx_NON_LEVELED_MASK | VSIE_MASK;



		static constexpr unsigned MIE_ADDR = 0x304;
		static constexpr unsigned MIEH_ADDR = 0x314;
		static constexpr unsigned SIE_ADDR = 0x104;
		static constexpr unsigned SIEH_ADDR = 0x114;
		static constexpr unsigned HIE_ADDR = 0x604;
		static constexpr unsigned VSIE_ADDR = 0x204;
		static constexpr unsigned VSIEH_ADDR = 0x214;
	} mie = xie(*this);

	struct xip : public csr64_bit_ops {
		csrs_clint_pend & clint;

		xip(struct csrs_clint_pend & clint) : clint(clint) {}

		union {
			uint64_t reg = 0;
			struct {
				unsigned wpri0 : 1;
				unsigned ssip : 1;
				unsigned vssip : 1;
				unsigned msip : 1;

				unsigned wpri1 : 1;
				unsigned stip : 1;
				unsigned vstip : 1;
				unsigned mtip : 1;

				unsigned wpri2 : 1;
				unsigned seip : 1;
				unsigned vseip : 1;
				unsigned meip : 1;

				unsigned sgeip : 1;

				unsigned wiri4 : 19;
				unsigned wiri5 : 32;
			} fields;
		};

		uint64_t mip_routed_read_64(void) {
			// all bits except HVIP_OR_MASK are just HW pending bits
			// HVIP_OR_MASK bits are ORed from hvip register with HW pending bits
			uint64_t mip_bits = reg | (clint.hvirt.hvip & csrs_hvirt::HVIP_OR_MASK);

			return read_64(mip_bits, MIP_READ_MASK);
		}

		uint64_t sip_routed_read_64(void) {
			auto [delegated, injected] = clint.s_route_masks();

			// Alias of mip[n]
			delegated &= mip_routed_read_64();
			// Alias of mvip[n]
			injected &= clint.mvirt.mvip;

			return read_64(delegated | injected, SIP_READ_MASK);
		}

		void sip_routed_write_64(uint64_t write_mask, uint64_t value) {
			auto [delegated, injected] = clint.s_route_masks();

			write_64(reg, write_mask & SIP_WRITE_DELEGATED_MASK & delegated, value);
			write_64(clint.mvirt.mvip, write_mask & SIP_WRITE_INJECTED_MASK & injected, value);
		}

		void checked_write_sip(uint32_t val) {
			sip_routed_write_64(WRITE_LO_MASK, lo_to_reg(val));
		}

		void checked_write_siph(uint32_t val) {
			sip_routed_write_64(WRITE_HI_MASK, hi_to_reg(val));
		}

		uint64_t vsip_routed_read_64(void) {
			// NOTE: lowest bits (HVIP_OR_MASK) are handled in mip read itself
			auto [delegated, injected] = clint.vs_route_masks();

			uint64_t sip = ~MIx_LEVELED_MASK & sip_routed_read_64();
			uint64_t hip = MIx_LEVELED_MASK & checked_read_hip();

			// Alias of sip[n] / hip[n]
			delegated = delegated & (sip | hip);
			// Alias of hvip[n]
			injected = injected & clint.hvirt.hvip;

			return read_64(delegated | injected, VSIP_READ_MASK);
		}

		template <uint64_t write_mask>
		void vsip_routed_write_64(uint64_t value) {
			value = vs_to_m_bits(value);

			auto [delegated, injected] = clint.vs_route_masks();

			uint64_t delegated_mask = write_mask & VSIP_WRITE_DELEGATED_MASK & delegated;

			sip_routed_write_64(delegated_mask, value);
			hip_routed_write_32(delegated_mask, value);
			write_64(clint.hvirt.hvip, write_mask & VSIP_WRITE_INJECTED_MASK & injected, value);
		}

		void checked_write_vsip(uint32_t val) {
			vsip_routed_write_64<WRITE_LO_MASK>(lo_to_reg(val));
		}

		void checked_write_vsiph(uint32_t val) {
			vsip_routed_write_64<WRITE_HI_MASK>(hi_to_reg(val));
		}

		/* low 32 bit registers xip */
		void checked_write_mip(uint32_t val) {
			write_lo(reg, MIP_WRITE_MASK, val);
		}

		void hip_routed_write_32(uint64_t write_mask, uint32_t val) {
			write_lo(reg, write_mask & HIP_WRITE_MASK, val);
		}

		void checked_write_hip(uint32_t val) {
			hip_routed_write_32(WRITE_LO_MASK, val);
		}

		uint32_t checked_read_mip(void) {
			return reg_to_lo(mip_routed_read_64());
		}

		uint32_t checked_read_sip(void) {
			return reg_to_lo(sip_routed_read_64());
		}

		uint32_t checked_read_hip(void) {
			// hip bits are always just aliases of mip
			return reg_to_lo(mip_routed_read_64() & HIP_READ_MASK);
		}

		uint32_t checked_read_vsip(void) {
			return reg_to_lo(m_to_vs_bits(vsip_routed_read_64()));
		}

		/* high 32 bit registers xip */
		void checked_write_miph(uint32_t val) {
			write_hi(reg, MIP_WRITE_MASK, val);
		}

		uint32_t checked_read_miph(void) {
			return reg_to_hi(mip_routed_read_64());
		}

		uint32_t checked_read_siph(void) {
			return reg_to_hi(sip_routed_read_64());
		}

		uint32_t checked_read_vsiph(void) {
			return reg_to_hi(m_to_vs_bits(vsip_routed_read_64()));
		}

		bool hw_write_mip(uint32_t iid, bool set) {
			uint64_t reg_before = reg;
			write_64(reg, MIP_READ_MASK & BIT(iid), set << iid);
			uint64_t reg_after = reg;

			// edge if bit go from 0 to 1
			bool edge = (reg_before & BIT(iid)) == 0 && (reg_after & BIT(iid)) == BIT(iid);

			return edge;
		}

		// xip
		static constexpr uint64_t MIP_WRITE_MASK = 0;
		static constexpr uint64_t MIP_READ_MASK = MIE_MASK;

		// TODO: even if delegated MIx_IRQ_TYPE_EDGE_MASK should be writable bits
		static constexpr uint64_t SIP_WRITE_DELEGATED_MASK = 0;
		static constexpr uint64_t SIP_WRITE_INJECTED_MASK = (MIx_NON_LEVELED_MASK | SIE_MASK) & MIx_IRQ_TYPE_EDGE_MASK;
		static constexpr uint64_t SIP_READ_MASK = MIx_NON_LEVELED_MASK | SIE_MASK;

		static constexpr uint64_t HIP_WRITE_MASK = 0;
		static constexpr uint64_t HIP_READ_MASK = HIE_MASK;

		// TODO: even if delegated MIx_IRQ_TYPE_EDGE_MASK should be writable bits
		static constexpr uint64_t VSIP_WRITE_DELEGATED_MASK = 0;
		static constexpr uint64_t VSIP_WRITE_INJECTED_MASK = (MIx_NON_LEVELED_MASK | VSIE_MASK) & MIx_IRQ_TYPE_EDGE_MASK;
		static constexpr uint64_t VSIP_READ_MASK = MIx_NON_LEVELED_MASK | VSIE_MASK;


		static constexpr unsigned MIP_ADDR = 0x344;
		static constexpr unsigned MIPH_ADDR = 0x354;
		static constexpr unsigned SIP_ADDR = 0x144;
		static constexpr unsigned SIPH_ADDR = 0x154;
		static constexpr unsigned HIP_ADDR = 0x644;
		static constexpr unsigned VSIP_ADDR = 0x244;
		static constexpr unsigned VSIPH_ADDR = 0x254;
	} mip = xip(*this);

	struct csrs_mvirt : public csr64_bit_ops {
		uint64_t mvien = 0;
		uint64_t mvip = 0;
		uint64_t shadow_enables = 0;

		// NOTE: mideleg doesn't affect mvien access
		// AIA: A bit in mvien can be set to 1 only for major interrupts 1, 9, and 13–63.
		// however, in RTIA we unify mvie & mvip behavior across bits
		static constexpr uint64_t MVIEN_MASK = MIx_NON_LEVELED_MASK | SIE_MASK;
		static constexpr uint64_t MVIP_MASK = MVIEN_MASK;

		// addrs
		static constexpr unsigned MVIP_ADDR = 0x309;
		static constexpr unsigned MVIPH_ADDR = 0x319;

		static constexpr unsigned MVIEN_ADDR = 0x308;
		static constexpr unsigned MVIENH_ADDR = 0x318;

		/* low 32 bit registers */
		void checked_write_mvien(uint32_t val) {
			write_lo(mvien, MVIEN_MASK, val);
		}

		uint32_t checked_read_mvien(void) {
			return read_lo(mvien, MVIEN_MASK);
		}

		void checked_write_mvip(uint32_t val) {
			write_lo(mvip, MVIP_MASK, val);
		}

		uint32_t checked_read_mvip(void) {
			return read_lo(mvip, MVIP_MASK);
		}

		/* high 32 bit registers */
		void checked_write_mvienh(uint32_t val) {
			write_hi(mvien, MVIEN_MASK, val);
		}

		uint32_t checked_read_mvienh(void) {
			return read_hi(mvien, MVIEN_MASK);
		}

		void checked_write_mviph(uint32_t val) {
			write_hi(mvip, MVIP_MASK, val);
		}

		uint32_t checked_read_mviph(void) {
			return read_hi(mvip, MVIP_MASK);
		}
	} mvirt;

	struct csrs_hvirt : public csr64_bit_ops {
		uint64_t hvien = 0;
		uint64_t hvip = 0;
		uint64_t shadow_enables = 0;

		// NOTE: hideleg doesn't affect hvien access
		// AIA: Bits 12:0 of hvien are reserved and must be read-only zeros,
		// while bits 12:0 of hvip are defined by the RISC-V Privileged Architecture.
		// Specifically, bits 10, 6, and 2 of hvip are writable bits that correspond
		// to VS-level external interrupts (VSEIP), VS-level timer interrupts (VSTIP),
		// and VS-level software interrupts (VSSIP), respectively.
		static constexpr uint64_t HVIEN_MASK = MIx_NON_LEVELED_MASK;
		static constexpr uint64_t HVIP_MASK = MIx_NON_LEVELED_MASK | MIx_VS_MASK;

		// legacy hvip bits from priveledged ISA
		static constexpr uint64_t HVIP_OR_MASK = VSIE_MASK;

		// addrs
		static constexpr unsigned HVIP_ADDR = 0x645;
		static constexpr unsigned HVIPH_ADDR = 0x655;

		static constexpr unsigned HVIEN_ADDR = 0x608;
		static constexpr unsigned HVIENH_ADDR = 0x618;

		/* low 32 bit registers */
		void checked_write_hvien(uint32_t val) {
			write_lo(hvien, HVIEN_MASK, val);
		}

		uint32_t checked_read_hvien(void) {
			return read_lo(hvien, HVIEN_MASK);
		}

		void checked_write_hvip(uint32_t val) {
			write_lo(hvip, HVIP_MASK, val);
		}

		uint32_t checked_read_hvip(void) {
			return read_lo(hvip, HVIP_MASK);
		}

		/* high 32 bit registers */
		void checked_write_hvienh(uint32_t val) {
			write_hi(hvien, HVIEN_MASK, val);
		}

		uint32_t checked_read_hvienh(void) {
			return read_hi(hvien, HVIEN_MASK);
		}

		void checked_write_hviph(uint32_t val) {
			write_hi(hvip, HVIP_MASK, val);
		}

		uint32_t checked_read_hviph(void) {
			return read_hi(hvip, HVIP_MASK);
		}
	} hvirt;

	struct csr_mideleg : public csr64_bit_ops {
		uint64_t checked_read_mideleg_64(void) {
			return read_64(reg, MIDELEG_READ_MASK);
		}

		void checked_write_mideleg(uint32_t val) {
			write_lo(reg, MIDELEG_WRITE_MASK, val);
		}

		uint32_t checked_read_mideleg(void) {
			return read_lo(reg, MIDELEG_READ_MASK);
		}

		void checked_write_midelegh(uint32_t val) {
			write_hi(reg, MIDELEG_WRITE_MASK, val);
		}

		uint32_t checked_read_midelegh(void) {
			return read_hi(reg, MIDELEG_READ_MASK);
		}

		static constexpr unsigned MIDELEG_ADDR = 0x303;
		static constexpr unsigned MIDELEGH_ADDR = 0x313;

		// When the hypervisor extension is implemented, bits 10, 6, and 2 of mideleg
		// (the standard VS-level interrupts) are each read-only one.
		// TODO: this should be configurable based on HS extension presence

		// If any guest external interrupts are implemented (GEILEN is nonzero), bit 12 of mideleg
		// (supervisor-level guest external interrupts) is also read-only one. VS-level interrupts
		// and guest external interrupts are always delegated past M-mode to HS-mode.
		static_assert(iss_config::MAX_GUEST != 0);

		uint64_t reg = HIE_MASK;

		// TODO: COUNTER_OVREFLOW_INTERRUPT - it should be delegatable?
		static constexpr uint64_t MIDELEG_READ_MASK = SIE_MASK | HIE_MASK;
		static constexpr uint64_t MIDELEG_WRITE_MASK = SIE_MASK;
	} mideleg;

	struct csr_hideleg : public csr64_bit_ops {
		csrs_clint_pend & clint;
		uint64_t reg = 0;

		csr_hideleg(struct csrs_clint_pend & clint) : clint(clint) {}

		uint64_t checked_read_hideleg_64(void) const {
			return read_64(reg, HIDELEG_MASK & hideleg_m_extra_mask());
		}

		void checked_write_hideleg(uint32_t val) {
			write_lo(reg, HIDELEG_MASK & hideleg_m_extra_mask(), val);
		}

		// TODO: what if we write 1 to some bit, masked it and unmasked it afterwards?
		// Should it be 0 or 1?
		uint32_t checked_read_hideleg(void) {
			return read_lo(reg, HIDELEG_MASK & hideleg_m_extra_mask());
		}

		void checked_write_hidelegh(uint32_t val) {
			write_hi(reg, HIDELEG_MASK & hideleg_m_extra_mask(), val);
		}

		uint32_t checked_read_hidelegh(void) {
			return read_hi(reg, HIDELEG_MASK & hideleg_m_extra_mask());
		}

		static constexpr unsigned HIDELEG_ADDR = 0x603;
		static constexpr unsigned HIDELEGH_ADDR = 0x613;

	private:
		// if a bit is zero in the same position in both mideleg
		// and mvien, then that bit is read-only zero in hideleg
		inline uint64_t hideleg_m_extra_mask(void) const {
			auto [delegated, injected] = clint.s_route_masks();

			return delegated | injected;
		}

		// Among bits 15:0 of hideleg, bits 10, 6, and 2
		// (corresponding to the standard VS-level interrupts) are writable
		// TODO: counter overflow
		static constexpr uint64_t HIDELEG_MASK = MIx_VS_MASK;

		// legacy masks
		static_assert(HIDELEG_MASK == 0b10001000100);
	} hideleg = csr_hideleg(*this);

	bool is_iid_injected(PrivilegeLevel level, uint32_t iid) {
		switch (level) {
			case MachineMode:
				return false;

			case SupervisorMode: {
				assert(major_irq::is_upper_bound_valid(iid));

				auto [delegated, injected] = s_route_masks();

				return !!(injected & BIT(iid));
			}

			case VirtualSupervisorMode:
				// TODO: this is not yet supported
				return false;

			default:
				throw std::runtime_error("unexpected privilege level " + std::to_string(level));
		}
	}

	inline std::tuple<uint64_t, uint64_t> s_route_masks(void) const {
		uint64_t delegated_mask = mideleg.reg;
		uint64_t injected_mask = ~mideleg.reg & mvirt.mvien;

		/* bug somewhere if intersect */
		assert(!(delegated_mask & injected_mask));

		return {delegated_mask, injected_mask};
	}

	inline std::tuple<uint64_t, uint64_t> vs_route_masks(void) const {
		uint64_t delegated_mask = hideleg.checked_read_hideleg_64();
		uint64_t injected_mask = ~delegated_mask & hvirt.hvien;
		// NOTE: bits 0-12 are delegated and injected simultaneously
		// (due to different injection mechanism). They are not set in
		// injected_mask

		/* bug somewhere if intersect */
		assert(!(delegated_mask & injected_mask));

		return {delegated_mask, injected_mask};
	}

	uint64_t s_irqs_present(void) const {
		auto [delegated, injected] = s_route_masks();

		return delegated | injected;
	}

	uint64_t vs_irqs_present(void) const {
		auto [delegated, injected] = vs_route_masks();

		return m_to_vs_bits(delegated | injected);
	}

	static uint64_t m_to_vs_bits(uint64_t mix) {
		uint64_t mix_vs = mix & MIx_VS_MASK;
		mix_vs >>= MIP_TO_VSIP_SHIFT;

		uint64_t mix_unleveled = mix & ~MIx_LEVELED_MASK;

		return mix_unleveled | mix_vs;
	}

	static uint64_t vs_to_m_bits(uint64_t vsix) {
		uint64_t vsix_vs = vsix & VSIx_VS_MASK;
		vsix_vs <<= MIP_TO_VSIP_SHIFT;

		uint64_t vsix_unleveled = vsix & ~MIx_LEVELED_MASK;

		return vsix_unleveled | vsix_vs;
	}

	static bool pendings_edge_detected(uint64_t pend_old, uint64_t pend_new, uint32_t iid) {
		return (pend_old & BIT(iid)) == 0 && (pend_new & BIT(iid)) == BIT(iid);
	}

	static constexpr uint32_t MIP_TO_VSIP_SHIFT = 1; // value we need to shift MIP to have VS bits on S bits place
};

struct csr_mepc {
	union {
		uint32_t reg = 0;
	};
};

struct csr_mcause {
	union {
		uint32_t reg = 0;
		struct {
			unsigned exception_code : 31;  // WLRL
			unsigned interrupt : 1;
		} fields;
	};
};

struct csr_mcounteren {
	union {
		uint32_t reg = 0;
		struct {
			unsigned CY : 1;
			unsigned TM : 1;
			unsigned IR : 1;
			unsigned reserved : 29;
		} fields;
	};
};

struct csr_mcountinhibit {
	union {
		uint32_t reg = 0;
		struct {
			unsigned CY : 1;
			unsigned zero : 1;
			unsigned IR : 1;
			unsigned reserved : 29;
		} fields;
	};
};

struct csr_pmpcfg {
	union {
		uint32_t reg = 0;
		struct {
			unsigned R0 : 1;              // WARL
			unsigned W0 : 1;              // WARL
			unsigned X0 : 1;              // WARL
			unsigned A0 : 2;              // WARL
			unsigned _wiri0 : 2;          // WIRI
			unsigned L0 : 1;              // WARL
			unsigned UNIMPLEMENTED : 24;  // WARL
		} fields;
	};
};

struct csr_satp {
	union {
		uint32_t reg = 0;
		struct {
			unsigned ppn : 22;  // WARL
			unsigned asid : 9;  // WARL
			unsigned mode : 1;  // WARL
		} fields;
	};
};

struct csr_hgatp {
	union {
		uint32_t reg = 0;
		struct {
			unsigned ppn : 22;  // WARL
			unsigned vmid : 7;  // WARL
			unsigned warl : 2;  // WARL
			unsigned mode : 1;  // WARL
		} fields;
	};
};

struct csr_fcsr {
	union {
		uint32_t reg = 0;
		struct {
			unsigned fflags : 5;
			unsigned frm : 3;
			unsigned reserved : 24;
		} fields;
		// fflags accessed separately
		struct {
			unsigned NX : 1;  // invalid operation
			unsigned UF : 1;  // divide by zero
			unsigned OF : 1;  // overflow
			unsigned DZ : 1;  // underflow
			unsigned NV : 1;  // inexact
			unsigned _ : 27;
		} fflags;
	};
};

struct csr_topi {
	union {
		uint32_t reg = 0;
		struct {
			unsigned iprio : 8;  // WARL
			unsigned wpri0 : 8;
			unsigned iid : 6;    // iid 0-63
			unsigned wpri1 : 11;
		} fields;
	};
};

struct csr_topei {
	union {
		uint32_t reg = 0;
		struct {
			unsigned iprio : 8;  // WARL
			unsigned wpri0 : 8;
			unsigned iid : 12;   // WARL
			unsigned wpri1 : 4;
		} fields;
	};
};

struct csr_hgeip : public csr_if {
	/* hgeip is read only */
	void checked_write(uint32_t val) override {}

	uint32_t checked_read(void) override {
		return reg & HGIP_MASK;
	}

	/* Bit indexing same as in vgein */
	void set_guest_pending(uint32_t id, bool val) {
		assert(id < iss_config::MAX_GUEST);

		uint32_t vgein = id_to_vgein(id);
		reg = (reg & ~BIT(vgein)) | (val << vgein);

		assert(!(reg & ~HGIP_MASK));
	}

	static constexpr unsigned HGEIP_ADDR = 0xE12;

private:
	uint32_t reg = 0;
	/* Bit indexing same as in vgein - starting with 1 */
	static constexpr uint32_t HGIP_MASK = GENMASK(iss_config::MAX_GUEST, 1);
};

struct csr_hgeie : public csr_if {
	void checked_write(uint32_t val) override {
		reg = (reg & ~HGEIE_MASK) | (val & HGEIE_MASK);
	}

	uint32_t checked_read(void) override {
		return reg & HGEIE_MASK;
	}

	static constexpr unsigned HGEIE_ADDR = 0x607;

private:
	uint32_t reg = 0;
	/* Bit indexing same as in vgein - starting with 1 */
	static constexpr uint32_t HGEIE_MASK = GENMASK(iss_config::MAX_GUEST, 1);
};

struct csr_hvictl : public csr_if {
	union {
		uint32_t reg = 0;
		struct {
			unsigned iprio : 8;
			unsigned ipriom : 1;
			unsigned dpr : 1;
			unsigned wpri0 : 6;
			unsigned iid : 12;	// WARL, iid limited to 0-63 by mask
			unsigned wpri1 : 2;
			unsigned vti : 1;
		} fields;
	};

	void checked_write(uint32_t val) override {
		reg = (reg & ~HVICTL_MASK) | (val & HVICTL_MASK);
	}

	uint32_t checked_read(void) override {
		return reg & HVICTL_MASK;
	}

	uint8_t get_s_iid(void) {
		return fields.iid;
	}

	uint8_t get_prio(void) {
		return fields.iprio;
	}

	bool is_vti_active(void) {
		return fields.vti;
	}

	bool is_external_injected(void) {
		return get_s_iid() == EXC_S_EXTERNAL_INTERRUPT && fields.iprio != 0;
	}

	bool is_local_injected(void) {
		return is_vti_active() && get_s_iid() != EXC_S_EXTERNAL_INTERRUPT;
	}

	bool is_ipriom_full_mode(void) {
		return fields.ipriom;
	}

	static constexpr unsigned HVICTL_ADDR = 0x609;

private:
	// iid limited to 0-63
	static constexpr uint32_t HVICTL_MASK = 0b1000000111111110000001111111111;
};

struct csr_menvcfg : public csr_if {
	union {
		uint32_t reg = 0;
		struct {
			unsigned fiom : 1;
			unsigned wpri1 : 3;
			unsigned cbie : 2;
			unsigned cbcfe : 1;
			unsigned cbze : 1;
			unsigned mtsp : 1;
			unsigned wpri2 : 23;
		} fields;
	};

	static constexpr unsigned MENVCFG_ADDR = 0x30A;

	void checked_write(uint32_t val) override {
		reg = (reg & ~MENVCFG_MASK) | (val & MENVCFG_MASK);
	}

	uint32_t checked_read(void) override {
		return reg & MENVCFG_MASK;
	}

private:
	static constexpr uint32_t MENVCFG_MASK = 0b1111110001;
};

struct csr_senvcfg : public csr_if {
	union {
		uint32_t reg = 0;
		struct {
			unsigned fiom : 1;
			unsigned wpri1 : 3;
			unsigned cbie : 2;
			unsigned cbcfe : 1;
			unsigned cbze : 1;
			unsigned stsp : 1;
			unsigned wpri2 : 23;
		} fields;
	};

	static constexpr unsigned SENVCFG_ADDR = 0x10A;

	void checked_write(uint32_t val) override {
		reg = (reg & ~SENVCFG_MASK) | (val & SENVCFG_MASK);
	}

	uint32_t checked_read(void) override {
		return reg & SENVCFG_MASK;
	}

private:
	static constexpr uint32_t SENVCFG_MASK = 0b111110001;
};

struct csr_henvcfg : public csr_if {
	union {
		uint32_t reg = 0;
		struct {
			unsigned fiom : 1;
			unsigned wpri1 : 3;
			unsigned cbie : 2;
			unsigned cbcfe : 1;
			unsigned cbze : 1;
			unsigned htsp : 1;
			unsigned vgtsp : 1;
			unsigned wpri2 : 22;
		} fields;
	};

	static constexpr unsigned HENVCFG_ADDR = 0x60A;

	void checked_write(uint32_t val) override {
		reg = (reg & ~HENVCFG_MASK) | (val & HENVCFG_MASK);
	}

	uint32_t checked_read(void) override {
		return reg & HENVCFG_MASK;
	}

private:
	static constexpr uint32_t HENVCFG_MASK = 0b1111110001;
};

// menvcfgh & henvcfgh have same layout for now
struct csr_xenvcfgh : public csr_if {
	union {
		uint32_t reg = 0;
		struct {
			unsigned wpri1 : 30;
			unsigned pbmte : 1;
			unsigned stce : 1;
		} fields;
	};

	csr_xenvcfgh(bool stce_present) {
		make_stce_present(stce_present);
	}

	static constexpr unsigned MENVCFGH_ADDR = 0x31A;
	static constexpr unsigned HENVCFGH_ADDR = 0x61A;

	void checked_write(uint32_t val) override {
		reg = (reg & ~mask) | (val & mask);
	}

	uint32_t checked_read(void) override {
		return reg & mask;
	}

	void make_stce_present(bool present) {
		if (present) {
			mask = xENVCFGH_MASK_W_STSE;
		} else {
			mask = xENVCFGH_MASK_WO_STSE;
			fields.stce = 0;
		}
	}

private:
	static constexpr uint32_t xENVCFGH_MASK_W_STSE = 0xC0000000;
	static constexpr uint32_t xENVCFGH_MASK_WO_STSE = 0x40000000;
	uint32_t mask = xENVCFGH_MASK_W_STSE;
};

/*
 * Add new subclasses with specific consistency check (e.g. by adding virtual
 * write_low, write_high functions) if necessary.
 */
struct csr_64 {
	union {
		uint64_t reg = 0;
		struct {
			int32_t low;
			int32_t high;
		} words;
	};

	void increment() {
		++reg;
	}
};

struct csr_timecontrol {
	union reg_64 {
		uint64_t reg = 0;
		struct {
			uint32_t low;
			uint32_t high;
		} words;
	};

	static constexpr unsigned TIME_ADDR = 0xC01;
	static constexpr unsigned TIMEH_ADDR = 0xC81;
	static constexpr unsigned HTIMEDELTA_ADDR = 0x605;
	static constexpr unsigned HTIMEDELTAH_ADDR = 0x615;

	static constexpr unsigned STIMECMP_ADDR = 0x14D;
	static constexpr unsigned STIMECMPH_ADDR = 0x15D;
	static constexpr unsigned VSTIMECMP_ADDR = 0x24D;
	static constexpr unsigned VSTIMECMPH_ADDR = 0x25D;

	union reg_64 time;
	union reg_64 htimedelta;

	union reg_64 stimecmp;
	union reg_64 vstimecmp;

	uint32_t read_time(bool from_virtual) {
		return get_time(from_virtual).words.low;
	}

	uint32_t read_timeh(bool from_virtual) {
		return get_time(from_virtual).words.high;
	}

	void update_time_counter(uint64_t new_value) {
		time.reg = new_value;
	}

	uint64_t get_timecmp_level_adjusted(PrivilegeLevel level) {
		if (level == SupervisorMode)
			return stimecmp.reg;
		else if (level == VirtualSupervisorMode)
			return vstimecmp_to_global_time();
		else
			assert(false);
	}

private:
	uint64_t vstimecmp_to_global_time(void) {
		return vstimecmp.reg - htimedelta.reg;
	}

	union reg_64 get_time(bool from_virtual) {
		if (from_virtual) {
			union reg_64 virtual_time;

			virtual_time.reg = time.reg + htimedelta.reg;
			return virtual_time;
		} else {
			return time;
		}
	}
};

struct icsr_if {
	virtual void checked_write(uint32_t val) = 0;
	virtual uint32_t checked_read(void) = 0;

	virtual ~icsr_if() {};
};

struct icsr_32 : public icsr_if {
	static constexpr uint32_t BITS_PER_CSR = 32;

	uint32_t reg = 0;
	uint32_t mask = 0xFFFFFFFF;

	void checked_write(uint32_t val) override {
		reg = (reg & ~mask) | (val & mask);
	}

	uint32_t checked_read(void) override {
		return reg & mask;
	}
};

struct icsr_eidelivery : public icsr_if {
	void checked_write(uint32_t val) override {
		reg = val & mask;
	}

	uint32_t checked_read(void) override {
		return reg & mask;
	}

	bool delivery_on(void) {
		return fields.delivery;
	}

private:
	union {
		uint32_t reg = 0;
		struct {
			unsigned delivery : 1;  // WARL
			unsigned wpri0 : 31;
		} fields;
	};

	uint32_t mask = 0x1;
};

struct icsr_eithreshold : public icsr_if {
	// constexpr uint32_t MAX_NV_VECTOR = 256;
	uint32_t reg = 0;

	void checked_write(uint32_t val) override {
		// If N is the maximum implemented interrupt identity number for this interrupt file,
		// eithreshold must be capable of holding all values between 0 and N, inclusive.
		if (is_upper_bound_valid_minor_iid(val)) {
			if (mode_snps_vectored) {
				write_snps_vectored(val);
			} else {
				reg = val;
			}
		}
	}

	uint32_t checked_read(void) override {
		if (mode_snps_vectored)
			return read_snps_vectored();
		else
			return reg;
	}

	void set_mode_snps_vectored(unsigned mode) {
		mode_snps_vectored = mode == csr_mtvec::SnpsNestedVectored;
	}

	void update_with_new_irq(uint32_t minor_iid) {
		if (mode_snps_vectored) {
			update_with_new_irq_snps_vectored(minor_iid);
		}
	}

	void mark_irq_as_handled(void) {
		tstack_pop();
	}

private:
	uint32_t read_snps_vectored(void) {
		return reg;
	}

	bool is_tailed_iid(uint32_t minor_iid) {
		return minor_iid > iss_config::NV_MODE_MAX_VECTOR;
	}

	void write_snps_vectored(uint32_t val) {
		if (val == 0) {
			tstack_pop();
		} else if (is_tailed_iid(val)) {
			tstack_put_tail(val);
		} else {
			tstack_insert(val);
		}
	}

	void update_with_new_irq_snps_vectored(uint32_t minor_iid) {
		if (is_tailed_iid(minor_iid)) {
			tstack_insert(iss_config::NV_MODE_MAX_VECTOR);
		} else {
			tstack_insert(minor_iid);
		}
	}

	bool mode_snps_vectored = false;

	bool is_reg_unstackable(void) {
		// 0 don't need to be put to thresholds array
		// tail iid stored separately and don't need to be put to thresholds array
		return reg == 0 || is_tailed_iid(reg);
	}

	void tstack_insert(uint32_t minor_iid) {
		assert(is_valid_minor_iid(minor_iid));
		assert(is_upper_bound_valid_minor_iid(reg));

		if (is_reg_unstackable()) {
			reg = minor_iid;
		} else {
			thresholds[reg] = true;

			if (reg > minor_iid) {
				reg = minor_iid;
			}
		}
	}

	void tstack_put_tail(uint32_t minor_iid) {
		threshold_tail = minor_iid;

		if (is_reg_unstackable()) {
			reg = minor_iid;
		}
	}

	void tstack_pop(void) {
		for (uint32_t i = 1; i <= iss_config::NV_MODE_MAX_VECTOR; i++) {
			if (thresholds[i]) {
				thresholds[i] = false;
				reg = i;

				return;
			}
		}

		// no entry in stack - tail value is active (if tail is 0 - all priorities allowed)
		reg = threshold_tail;
	}

	bool thresholds[iss_config::NV_MODE_MAX_VECTOR + 1] = {};
	uint32_t threshold_tail = 0;
};

struct iprio_reg {
	static constexpr uint32_t PRIO_PER_IPRIO_CSR = 4;

	union iprio_arr {
		uint32_t reg = 0;
		uint8_t array[PRIO_PER_IPRIO_CSR];
		struct {
			unsigned prio0 : 8;  // WARL
			unsigned prio1 : 8;  // WARL
			unsigned prio2 : 8;  // WARL
			unsigned prio3 : 8;  // WARL
		} fields;
	};
};

struct icsr_iprio_arr {
	struct icsr_iprio : public icsr_if, public iprio_reg {

		icsr_iprio() : base(nullptr), first_iid(0) {};
		icsr_iprio(struct icsr_iprio_arr * base, uint32_t index) : base(base), first_iid(index * PRIO_PER_IPRIO_CSR) {};

		void checked_write(uint32_t val) override {
			union iprio_arr reg;

			reg.reg = val;

			for (unsigned i = 0; i < PRIO_PER_IPRIO_CSR; i++)
				base->set_iprio(first_iid + i, reg.array[i]);
		}

		uint32_t checked_read(void) override {
			union iprio_arr reg;

			for (unsigned i = 0; i < PRIO_PER_IPRIO_CSR; i++)
				reg.array[i] = base->get_iprio(first_iid + i);

			return reg.reg;
		}

	private:
		struct icsr_iprio_arr * base;
		uint32_t first_iid;
	};

	void map(std::unordered_map<unsigned, icsr_if *> & mapping) {
		for (unsigned i = 0; i < iprio_csr_arr_size; ++i) {
			iprio_icsr_regs[i] = icsr_iprio(this, i);
			mapping[icsr_addr_iprio0 + i] = &iprio_icsr_regs[i];
		}
	}

	void mark_present_in_hw(uint32_t iid) {
		assert(major_irq::is_valid(iid));

		static_mask |= BIT(iid);
	}

	void update_dynamic_presence(uint64_t new_mask) {
		dynamic_mask = new_mask;
	}

	uint8_t get_iprio(uint32_t iid) {
		if (is_iprio_present(iid)) {
			return iprio[iid];
		} else {
			return 0;
		}
	}

	void set_iprio(uint32_t iid, uint8_t value) {
		if (is_iprio_present(iid)) {
			iprio[iid] = value;
		}
	}

	// addrs
	static constexpr unsigned icsr_addr_iprio0 = 0x30;

private:
	bool is_iprio_present(uint32_t iid) {
		assert(major_irq::is_valid(iid));

		return !!(static_mask & dynamic_mask & BIT(iid));
	}

	uint64_t static_mask = 0, dynamic_mask = UINT64_MAX;

	static constexpr unsigned iprio_num = major_irq::MAX_INTERRUPTS_NUM;
	static constexpr unsigned iprio_csr_arr_size = iprio_num / iprio_reg::PRIO_PER_IPRIO_CSR;

	icsr_iprio iprio_icsr_regs[iprio_csr_arr_size];

	uint8_t iprio[iprio_num] = { 0 };
};

struct vs_iprio_banks {
	// VS iprio banks. One for each guest and one for VGEIN = 0
	icsr_iprio_arr iprio[csr_hstatus::MAX_VGEIN_BANKS];

	vs_iprio_banks() {
		for (unsigned i = 0; i < csr_hstatus::MAX_VGEIN_BANKS; i++) {
			iprio[i].mark_present_in_hw(EXC_S_SOFTWARE_INTERRUPT);
			iprio[i].mark_present_in_hw(EXC_S_TIMER_INTERRUPT);
			iprio[i].mark_present_in_hw(EXC_COUNTER_OVREFLOW_INTERRUPT);
			iprio[i].mark_present_in_hw(EXC_DEBUG_TRACE_INTERRUPT);
			iprio[i].mark_present_in_hw(EXC_LOW_PRIO_RAS_INTERRUPT);
			iprio[i].mark_present_in_hw(EXC_HIGH_PRIO_RAS_INTERRUPT);
			iprio[i].mark_present_in_hw(EXC_WDT_INTERRUPT);
		}
	}

	void map(std::unordered_map<unsigned, icsr_if *> (& mapping)[csr_hstatus::MAX_VGEIN_BANKS]) {
		for (unsigned i = 0; i < csr_hstatus::MAX_VGEIN_BANKS; i++) {
			iprio[i].map(mapping[i]);
		}
	}

	void update_dynamic_presence(uint64_t new_mask) {
		for (auto & bank : iprio) {
			bank.update_dynamic_presence(new_mask);
		}
	}

	icsr_iprio_arr & operator[](int vgein) {
		return iprio[vgein];
	}

	struct hviprio {
		hviprio(vs_iprio_banks & iprio, uint8_t iid0, uint8_t iid1, uint8_t iid2, uint8_t iid3) :
			iprio(iprio), iid0(iid0), iid1(iid1), iid2(iid2), iid3(iid3) {}

		void checked_write(uint32_t val, int vgein) {
			union iprio_reg::iprio_arr reg;

			reg.reg = val;

			iprio[vgein].set_iprio(iid0, reg.array[0]);
			iprio[vgein].set_iprio(iid1, reg.array[1]);
			iprio[vgein].set_iprio(iid2, reg.array[2]);
			iprio[vgein].set_iprio(iid3, reg.array[3]);
		}

		uint32_t checked_read(int vgein) {
			union iprio_reg::iprio_arr reg;

			reg.array[0] = iprio[vgein].get_iprio(iid0);
			reg.array[1] = iprio[vgein].get_iprio(iid1);
			reg.array[2] = iprio[vgein].get_iprio(iid2);
			reg.array[3] = iprio[vgein].get_iprio(iid3);

			return reg.reg;
		}

	private:
		vs_iprio_banks & iprio;
		const uint8_t iid0, iid1, iid2, iid3;
	};

	static constexpr unsigned HVIPRIO1_ADDR = 0x646;
	static constexpr unsigned HVIPRIO1H_ADDR = 0x656;

	struct hviprio hviprio1 = hviprio(*this, 0, EXC_S_SOFTWARE_INTERRUPT, 4, EXC_S_TIMER_INTERRUPT);
	struct hviprio hviprio1h = hviprio(*this, 8, EXC_COUNTER_OVREFLOW_INTERRUPT, 14, 15);

	static constexpr unsigned HVIPRIO2_ADDR = 0x647;
	static constexpr unsigned HVIPRIO2H_ADDR = 0x657;

	struct hviprio hviprio2 = hviprio(*this, 16, EXC_DEBUG_TRACE_INTERRUPT, 18, 19);
	struct hviprio hviprio2h = hviprio(*this, 20, 21, 22, 23);
};

namespace icsr {
constexpr unsigned icsr_addr_smpuaddr0  = 0x100;
constexpr unsigned icsr_addr_smpuconf0  = 0x101;
constexpr unsigned icsr_addr_smpuaddr1  = 0x102;
constexpr unsigned icsr_addr_smpuconf1  = 0x103;
constexpr unsigned icsr_addr_smpuaddr2  = 0x104;
constexpr unsigned icsr_addr_smpuconf2  = 0x105;
constexpr unsigned icsr_addr_smpuaddr3  = 0x106;
constexpr unsigned icsr_addr_smpuconf3  = 0x107;
constexpr unsigned icsr_addr_smpuaddr4  = 0x108;
constexpr unsigned icsr_addr_smpuconf4  = 0x109;
constexpr unsigned icsr_addr_smpuaddr5  = 0x10A;
constexpr unsigned icsr_addr_smpuconf5  = 0x10B;
constexpr unsigned icsr_addr_smpuaddr6  = 0x10C;
constexpr unsigned icsr_addr_smpuconf6  = 0x10D;
constexpr unsigned icsr_addr_smpuaddr7  = 0x10E;
constexpr unsigned icsr_addr_smpuconf7  = 0x10F;
constexpr unsigned icsr_addr_smpuaddr8  = 0x110;
constexpr unsigned icsr_addr_smpuconf8  = 0x111;
constexpr unsigned icsr_addr_smpuaddr9  = 0x112;
constexpr unsigned icsr_addr_smpuconf9  = 0x113;
constexpr unsigned icsr_addr_smpuaddr10 = 0x114;
constexpr unsigned icsr_addr_smpuconf10 = 0x115;
constexpr unsigned icsr_addr_smpuaddr11 = 0x116;
constexpr unsigned icsr_addr_smpuconf11 = 0x117;
constexpr unsigned icsr_addr_smpuaddr12 = 0x118;
constexpr unsigned icsr_addr_smpuconf12 = 0x119;
constexpr unsigned icsr_addr_smpuaddr13 = 0x11A;
constexpr unsigned icsr_addr_smpuconf13 = 0x11B;
constexpr unsigned icsr_addr_smpuaddr14 = 0x11C;
constexpr unsigned icsr_addr_smpuconf14 = 0x11D;
constexpr unsigned icsr_addr_smpuaddr15 = 0x11E;
constexpr unsigned icsr_addr_smpuconf15 = 0x11F;
constexpr unsigned icsr_addr_smpuaddr16 = 0x120;
constexpr unsigned icsr_addr_smpuconf16 = 0x121;
constexpr unsigned icsr_addr_smpuaddr17 = 0x122;
constexpr unsigned icsr_addr_smpuconf17 = 0x123;
constexpr unsigned icsr_addr_smpuaddr18 = 0x124;
constexpr unsigned icsr_addr_smpuconf18 = 0x125;
constexpr unsigned icsr_addr_smpuaddr19 = 0x126;
constexpr unsigned icsr_addr_smpuconf19 = 0x127;
constexpr unsigned icsr_addr_smpuaddr20 = 0x128;
constexpr unsigned icsr_addr_smpuconf20 = 0x129;
constexpr unsigned icsr_addr_smpuaddr21 = 0x12A;
constexpr unsigned icsr_addr_smpuconf21 = 0x12B;
constexpr unsigned icsr_addr_smpuaddr22 = 0x12C;
constexpr unsigned icsr_addr_smpuconf22 = 0x12D;
constexpr unsigned icsr_addr_smpuaddr23 = 0x12E;
constexpr unsigned icsr_addr_smpuconf23 = 0x12F;
constexpr unsigned icsr_addr_smpuaddr24 = 0x130;
constexpr unsigned icsr_addr_smpuconf24 = 0x131;
constexpr unsigned icsr_addr_smpuaddr25 = 0x132;
constexpr unsigned icsr_addr_smpuconf25 = 0x133;
constexpr unsigned icsr_addr_smpuaddr26 = 0x134;
constexpr unsigned icsr_addr_smpuconf26 = 0x135;
constexpr unsigned icsr_addr_smpuaddr27 = 0x136;
constexpr unsigned icsr_addr_smpuconf27 = 0x137;
constexpr unsigned icsr_addr_smpuaddr28 = 0x138;
constexpr unsigned icsr_addr_smpuconf28 = 0x139;
constexpr unsigned icsr_addr_smpuaddr29 = 0x13A;
constexpr unsigned icsr_addr_smpuconf29 = 0x13B;
constexpr unsigned icsr_addr_smpuaddr30 = 0x13C;
constexpr unsigned icsr_addr_smpuconf30 = 0x13D;
constexpr unsigned icsr_addr_smpuaddr31 = 0x13E;
constexpr unsigned icsr_addr_smpuconf31 = 0x13F;
}

inline bool is_smpuaddr(unsigned addr) {
	return addr >= icsr::icsr_addr_smpuaddr0 && addr <= icsr::icsr_addr_smpuconf31;
}

struct csr_smpumask {
	uint32_t reg = 0;

	bool is_set_for_addr(unsigned icsr_addr) {
		unsigned int nbit = (icsr_addr - icsr::icsr_addr_smpuaddr0) >> 1;

		return reg & (1 << nbit);
	}
};

struct csr_table {
	csr_timecontrol timecontrol;

	csr_64 cycle;
	csr_64 instret;

	csr_mvendorid mvendorid;
	csr_32 marchid;
	csr_32 mimpid;
	csr_32 mhartid;

	csr_mstatus mstatus;
	csr_mstatush mstatush;
	csr_misa misa;
	csr_32 medeleg;
	csr_mtvec mtvec;
	csr_mcounteren mcounteren;
	csr_mcountinhibit mcountinhibit;

	csr_menvcfg menvcfg;
	csr_xenvcfgh menvcfgh = csr_xenvcfgh(true);

	csr_32 mscratch;
	csr_mepc mepc;
	csr_mcause mcause;
	csr_32 mtval;
	csr_32 mtval2;
	csr_32 mtinst;
	csrs_clint_pend clint;
	csr_32 mtsp;

	csr_32 miselect;
	csr_32 mireg;
	csr_32 mireg2;
	csr_32 mireg3;
	csr_32 mireg4;
	csr_32 mireg5;
	csr_32 mireg6;

	csr_topi mtopi;
	csr_topei mtopei;

	// pmp configuration
	std::array<csr_32, 16> pmpaddr;
	std::array<csr_pmpcfg, 4> pmpcfg;

	// spmp configuration
	std::array<csr_32, 64> spmpaddr;
	std::array<csr_32, 16> spmpcfg;
	std::array<csr_32, 2> spmpswitch;

	// smpu configuration
	csr_smpumask smpumask;

	// supervisor csrs (please note: some are already covered by the machine mode csrs, i.e. sstatus, sie and sip, and
	// some are required but have the same fields, hence the machine mode classes are used)
	csr_mtvec stvec;
	csr_mcounteren scounteren;
	csr_32 sscratch;
	csr_mepc sepc;
	csr_mcause scause;
	csr_32 stval;
	csr_satp satp;
	csr_32 stsp;
	csr_senvcfg senvcfg;

	csr_32 siselect;
	csr_32 sireg;
	csr_32 sireg2;
	csr_32 sireg3;
	csr_32 sireg4;
	csr_32 sireg5;
	csr_32 sireg6;

	csr_topi stopi;
	csr_topei stopei;

	// H-extended supervisor csrs
	csr_hstatus hstatus;
	csr_32 hcontext;
	csr_32 hedeleg;
	csr_hvictl hvictl;
	csr_hgeip hgeip;
	csr_hgeie hgeie;
	csr_64 htimedelta;
	csr_32 htsp;
	csr_hgatp hgatp;
	csr_32 hmpumask;
	csr_32 htval;
	csr_32 htinst;

	csr_henvcfg henvcfg;
	csr_xenvcfgh henvcfgh = csr_xenvcfgh(false);

	// VS csrs
	csr_mtvec vstvec;
	csr_mepc vsepc;
	csr_mcause vscause;
	csr_32 vstval;
	csr_32 vsscratch;
	csr_vsstatus vsstatus;
	csr_32 vstsp;

	csr_32 vsiselect;
	csr_32 vsireg;
	csr_32 vsireg2;
	csr_32 vsireg3;
	csr_32 vsireg4;
	csr_32 vsireg5;
	csr_32 vsireg6;

	csr_topi vstopi;
	csr_topei vstopei;

	csr_satp vsatp;
	csr_smpumask vsmpumask; // HS smpu extension

	// user csrs (see above comment)
	csr_fcsr fcsr;

	std::unordered_map<unsigned, uint32_t *> register_mapping;

	csr_table() {
		using namespace csr;

		register_mapping[CYCLE_ADDR] = (uint32_t *)(&cycle.reg);
		register_mapping[CYCLEH_ADDR] = (uint32_t *)(&cycle.reg) + 1;
		register_mapping[INSTRET_ADDR] = (uint32_t *)(&instret.reg);
		register_mapping[INSTRETH_ADDR] = (uint32_t *)(&instret.reg) + 1;
		register_mapping[MCYCLE_ADDR] = (uint32_t *)(&cycle.reg);
		register_mapping[MCYCLEH_ADDR] = (uint32_t *)(&cycle.reg) + 1;
		register_mapping[MINSTRET_ADDR] = (uint32_t *)(&instret.reg);
		register_mapping[MINSTRETH_ADDR] = (uint32_t *)(&instret.reg) + 1;

		register_mapping[MVENDORID_ADDR] = &mvendorid.reg;
		register_mapping[MARCHID_ADDR] = &marchid.reg;
		register_mapping[MIMPID_ADDR] = &mimpid.reg;
		register_mapping[MHARTID_ADDR] = &mhartid.reg;

		register_mapping[MSTATUS_ADDR] = &mstatus.reg;
		register_mapping[MSTATUSH_ADDR] = &mstatush.reg;
		register_mapping[MISA_ADDR] = &misa.reg;
		register_mapping[MEDELEG_ADDR] = &medeleg.reg;
		register_mapping[MTVEC_ADDR] = &mtvec.reg;
		register_mapping[MCOUNTEREN_ADDR] = &mcounteren.reg;
		register_mapping[MCOUNTINHIBIT_ADDR] = &mcountinhibit.reg;

		register_mapping[MSCRATCH_ADDR] = &mscratch.reg;
		register_mapping[MEPC_ADDR] = &mepc.reg;
		register_mapping[MCAUSE_ADDR] = &mcause.reg;
		register_mapping[MTVAL_ADDR] = &mtval.reg;
		register_mapping[MTVAL2_ADDR] = &mtval2.reg;
		register_mapping[MTINST_ADDR] = &mtinst.reg;
		register_mapping[MTSP_ADDR] = &mtsp.reg;

		register_mapping[MISELECT_ADDR] = &miselect.reg;
		register_mapping[MIREG_ADDR] = &mireg.reg;
		register_mapping[MIREG2_ADDR] = &mireg2.reg;
		register_mapping[MIREG3_ADDR] = &mireg3.reg;
		register_mapping[MIREG4_ADDR] = &mireg4.reg;
		register_mapping[MIREG5_ADDR] = &mireg5.reg;
		register_mapping[MIREG6_ADDR] = &mireg6.reg;

		register_mapping[MTOPI_ADDR] = &mtopi.reg;
		register_mapping[MTOPEI_ADDR] = &mtopei.reg;

		for (unsigned i = 0; i < 16; ++i) register_mapping[PMPADDR0_ADDR + i] = &pmpaddr[i].reg;

		for (unsigned i = 0; i < 4; ++i) register_mapping[PMPCFG0_ADDR + i] = &pmpcfg[i].reg;

		for (unsigned i = 0; i < 64; ++i) register_mapping[SPMPADDR0_ADDR + i] = &spmpaddr[i].reg;

		for (unsigned i = 0; i < 16; ++i) register_mapping[SPMPCFG0_ADDR + i] = &spmpcfg[i].reg;

		for (unsigned i = 0; i < 2; ++i) register_mapping[SPMPSWITCH0_ADDR + i] = &spmpswitch[i].reg;

		register_mapping[SMPUMASK_ADDR] = &smpumask.reg;

		register_mapping[STVEC_ADDR] = &stvec.reg;
		register_mapping[SCOUNTEREN_ADDR] = &scounteren.reg;
		register_mapping[SSCRATCH_ADDR] = &sscratch.reg;
		register_mapping[SEPC_ADDR] = &sepc.reg;
		register_mapping[SCAUSE_ADDR] = &scause.reg;
		register_mapping[STVAL_ADDR] = &stval.reg;
		register_mapping[SATP_ADDR] = &satp.reg;
		register_mapping[STSP_ADDR] = &stsp.reg;

		register_mapping[SISELECT_ADDR] = &siselect.reg;
		register_mapping[SIREG_ADDR] = &sireg.reg;
		register_mapping[SIREG2_ADDR] = &sireg2.reg;
		register_mapping[SIREG3_ADDR] = &sireg3.reg;
		register_mapping[SIREG4_ADDR] = &sireg4.reg;
		register_mapping[SIREG5_ADDR] = &sireg5.reg;
		register_mapping[SIREG6_ADDR] = &sireg6.reg;

		register_mapping[STOPI_ADDR] = &stopi.reg;
		register_mapping[STOPEI_ADDR] = &stopei.reg;

		register_mapping[HSTATUS_ADDR] = &hstatus.reg;
		register_mapping[HEDELEG_ADDR] = &hedeleg.reg;
		register_mapping[HCONTEXT_ADDR] = &hcontext.reg;
		register_mapping[HTSP_ADDR] = &htsp.reg;
		register_mapping[HGATP_ADDR] = &hgatp.reg;
		register_mapping[HMPUMASK_ADDR] = &hmpumask.reg;
		register_mapping[HTVAL_ADDR] = &htval.reg;
		register_mapping[HTINST_ADDR] = &htinst.reg;

		register_mapping[VSTVEC_ADDR] = &vstvec.reg;
		register_mapping[VSEPC_ADDR] = &vsepc.reg;
		register_mapping[VSCAUSE_ADDR] = &vscause.reg;
		register_mapping[VSTVAL_ADDR] = &vstval.reg;
		register_mapping[VSSCRATCH_ADDR] = &vsscratch.reg;
		register_mapping[VSSTATUS_ADDR] = &vsstatus.reg;
		register_mapping[VSTSP_ADDR] = &vstsp.reg;

		register_mapping[VSISELECT_ADDR] = &vsiselect.reg;
		register_mapping[VSIREG_ADDR] = &vsireg.reg;
		register_mapping[VSIREG2_ADDR] = &vsireg2.reg;
		register_mapping[VSIREG3_ADDR] = &vsireg3.reg;
		register_mapping[VSIREG4_ADDR] = &vsireg4.reg;
		register_mapping[VSIREG5_ADDR] = &vsireg5.reg;
		register_mapping[VSIREG6_ADDR] = &vsireg6.reg;

		register_mapping[VSTOPI_ADDR] = &vstopi.reg;
		register_mapping[VSTOPEI_ADDR] = &vstopei.reg;

		register_mapping[VSMPUMASK_ADDR] = &vsmpumask.reg;
		register_mapping[VSATP_ADDR] = &vsatp.reg;

		register_mapping[FCSR_ADDR] = &fcsr.reg;
	}

	bool is_valid_csr32_addr(unsigned addr) {
		return register_mapping.find(addr) != register_mapping.end();
	}

	void default_write32(unsigned addr, uint32_t value) {
		auto it = register_mapping.find(addr);
		ensure((it != register_mapping.end()) && "validate address before calling this function");
		*it->second = value;
	}

	uint32_t default_read32(unsigned addr) {
		auto it = register_mapping.find(addr);
		ensure((it != register_mapping.end()) && "validate address before calling this function");
		return *it->second;
	}
};


namespace icsr {
constexpr unsigned icsr_addr_eidelivery = 0x70;
constexpr unsigned icsr_addr_eithreshold = 0x72;
constexpr unsigned icsr_addr_eip0 = 0x80;
constexpr unsigned icsr_addr_eie0 = 0xC0;
}

struct icsr_smpuaddr : public icsr_if {
	enum {
		SMPU_ACCES_DENIED = 0,
		SMPU_ACCES_GRANTED,
		SMPU_ACCES_GRANTED_FOR_FIRST_HALF,
		SMPU_ACCES_GRANTED_FOR_SECOND_HALF
	};

	enum {
		SMPU_NAPOT_MIN_ALIGN_FACTOR = 8,
		SMPU_NAPOT_MAX_ALIGN_FACTOR = 31
	};

	void checked_write(uint32_t val) override {
		uint32_t size;

		reg = val;
		size = fields_atx.size;
		if (size) {
			/* Support any NAPOT region size in a range from 256 bytes (n = 8)
			 * to 2 GiB (n = 31)
			 */
			if (size < SMPU_NAPOT_MIN_ALIGN_FACTOR )
				size = SMPU_NAPOT_MIN_ALIGN_FACTOR;
			if (size > SMPU_NAPOT_MAX_ALIGN_FACTOR)
				size = SMPU_NAPOT_MAX_ALIGN_FACTOR;
			reg = (reg >> size) << size;
			fields_atx.size = size;
		} else {
			fields.reserv = 0;
		}
	}

	uint32_t checked_read(void) override {
		return reg;
	}

	uint32_t get_addr(void) {
		return fields.addr << 5; /* Address alignment factor n = 5 */
	}

	bool is_translated_region(void) {
		return fields_atx.size;
	}

	int check_translated_is_matched(uint32_t addr_start, uint32_t addr_end) {
		uint32_t val;

		addr_start >>= fields_atx.size;
		addr_end >>= fields_atx.size;
		val = reg >> fields_atx.size; /*ToDo: Can be optimized */
		if ((addr_start == val) && (addr_end == val)) {
			return SMPU_ACCES_GRANTED;
		} else if (addr_start == val) {
			return SMPU_ACCES_GRANTED_FOR_FIRST_HALF;
		} else if (addr_end == val) {
			return SMPU_ACCES_GRANTED_FOR_SECOND_HALF;
		}
		return SMPU_ACCES_DENIED;
	}

	uint32_t get_n(void) {
		return fields_atx.size;
	}

private:
	union {
		uint32_t reg = 0;
		struct {
			unsigned reserv : 5;  // RAZ
			unsigned addr   : 27;  // WARL
		} fields; /* S-mode MPU Extension */
		struct {
			unsigned size   : 5;  // WARL
			unsigned reserv : 3;  // RAZ
			unsigned vaddr  : 24;  // WARL
		} fields_atx; /* S-mode MPU Address Translation Extension */
	};
};

struct icsr_smpuconf : public icsr_if {
	/* ToDo: clear(RAZ) bits [n-1:8], where n = icsr_smpuaddr.size.
	 * These bits are currently ignored
	 */
	void checked_write(uint32_t val) override {
		reg = val;
	}

	uint32_t checked_read(void) override {
		return reg;
	}

	uint32_t get_size(void) {
		return (fields.size << 5) | 0x1F; /* Address alignment factor n = 5 */
	}

	/* The same for translated and not translated */
	uint32_t get_attr(void) {
		return (fields.U << 3) | (fields.R << 2) | (fields.W << 1) | fields.X;
	}

	uint32_t get_paddr(void) {
		return fields_atx.paddr << 8;
	}

	uint32_t get_pax(void) {
		return fields_atx.pax;
	}

	union {
		uint32_t reg = 0;
		struct {
			unsigned X : 1;  // WARL
			unsigned W : 1;  // WARL
			unsigned R : 1;  // WARL
			unsigned U : 1;  // WARL
			unsigned reserv : 1;  // RAZ
			unsigned size : 27;  // WARL
		} fields; /* S-mode MPU Extension */
		struct {
			unsigned X : 1;  // WARL
			unsigned W : 1;  // WARL
			unsigned R : 1;  // WARL
			unsigned U : 1;  // WARL
			unsigned reserv : 2;  // RAZ
			unsigned pax : 2;  // WARL
			unsigned paddr : 24;  // WARL
		} fields_atx; /* S-mode MPU Address Translation Extension */
	};
};

namespace icsr {
constexpr unsigned icsr_addr_hmpuaddr0  = 0x180;
constexpr unsigned icsr_addr_hmpuconf0  = 0x181;
constexpr unsigned icsr_addr_hmpuaddr1  = 0x182;
constexpr unsigned icsr_addr_hmpuconf1  = 0x183;
constexpr unsigned icsr_addr_hmpuaddr2  = 0x184;
constexpr unsigned icsr_addr_hmpuconf2  = 0x185;
constexpr unsigned icsr_addr_hmpuaddr3  = 0x186;
constexpr unsigned icsr_addr_hmpuconf3  = 0x187;
constexpr unsigned icsr_addr_hmpuaddr4  = 0x188;
constexpr unsigned icsr_addr_hmpuconf4  = 0x189;
constexpr unsigned icsr_addr_hmpuaddr5  = 0x18A;
constexpr unsigned icsr_addr_hmpuconf5  = 0x18B;
constexpr unsigned icsr_addr_hmpuaddr6  = 0x18C;
constexpr unsigned icsr_addr_hmpuconf6  = 0x18D;
constexpr unsigned icsr_addr_hmpuaddr7  = 0x18E;
constexpr unsigned icsr_addr_hmpuconf7  = 0x18F;
constexpr unsigned icsr_addr_hmpuaddr8  = 0x190;
constexpr unsigned icsr_addr_hmpuconf8  = 0x191;
constexpr unsigned icsr_addr_hmpuaddr9  = 0x192;
constexpr unsigned icsr_addr_hmpuconf9  = 0x193;
constexpr unsigned icsr_addr_hmpuaddr10 = 0x194;
constexpr unsigned icsr_addr_hmpuconf10 = 0x195;
constexpr unsigned icsr_addr_hmpuaddr11 = 0x196;
constexpr unsigned icsr_addr_hmpuconf11 = 0x197;
constexpr unsigned icsr_addr_hmpuaddr12 = 0x198;
constexpr unsigned icsr_addr_hmpuconf12 = 0x199;
constexpr unsigned icsr_addr_hmpuaddr13 = 0x19A;
constexpr unsigned icsr_addr_hmpuconf13 = 0x19B;
constexpr unsigned icsr_addr_hmpuaddr14 = 0x19C;
constexpr unsigned icsr_addr_hmpuconf14 = 0x19D;
constexpr unsigned icsr_addr_hmpuaddr15 = 0x19E;
constexpr unsigned icsr_addr_hmpuconf15 = 0x19F;
constexpr unsigned icsr_addr_hmpuaddr16 = 0x1A0;
constexpr unsigned icsr_addr_hmpuconf16 = 0x1A1;
constexpr unsigned icsr_addr_hmpuaddr17 = 0x1A2;
constexpr unsigned icsr_addr_hmpuconf17 = 0x1A3;
constexpr unsigned icsr_addr_hmpuaddr18 = 0x1A4;
constexpr unsigned icsr_addr_hmpuconf18 = 0x1A5;
constexpr unsigned icsr_addr_hmpuaddr19 = 0x1A6;
constexpr unsigned icsr_addr_hmpuconf19 = 0x1A7;
constexpr unsigned icsr_addr_hmpuaddr20 = 0x1A8;
constexpr unsigned icsr_addr_hmpuconf20 = 0x1A9;
constexpr unsigned icsr_addr_hmpuaddr21 = 0x1AA;
constexpr unsigned icsr_addr_hmpuconf21 = 0x1AB;
constexpr unsigned icsr_addr_hmpuaddr22 = 0x1AC;
constexpr unsigned icsr_addr_hmpuconf22 = 0x1AD;
constexpr unsigned icsr_addr_hmpuaddr23 = 0x1AE;
constexpr unsigned icsr_addr_hmpuconf23 = 0x1AF;
constexpr unsigned icsr_addr_hmpuaddr24 = 0x1B0;
constexpr unsigned icsr_addr_hmpuconf24 = 0x1B1;
constexpr unsigned icsr_addr_hmpuaddr25 = 0x1B2;
constexpr unsigned icsr_addr_hmpuconf25 = 0x1B3;
constexpr unsigned icsr_addr_hmpuaddr26 = 0x1B4;
constexpr unsigned icsr_addr_hmpuconf26 = 0x1B5;
constexpr unsigned icsr_addr_hmpuaddr27 = 0x1B6;
constexpr unsigned icsr_addr_hmpuconf27 = 0x1B7;
constexpr unsigned icsr_addr_hmpuaddr28 = 0x1B8;
constexpr unsigned icsr_addr_hmpuconf28 = 0x1B9;
constexpr unsigned icsr_addr_hmpuaddr29 = 0x1BA;
constexpr unsigned icsr_addr_hmpuconf29 = 0x1BB;
constexpr unsigned icsr_addr_hmpuaddr30 = 0x1BC;
constexpr unsigned icsr_addr_hmpuconf30 = 0x1BD;
constexpr unsigned icsr_addr_hmpuaddr31 = 0x1BE;
constexpr unsigned icsr_addr_hmpuconf31 = 0x1BF;
}

// risc-v inderect CSR access extension(Smcsrind)
struct icsr_ms_table {
	std::unordered_map<unsigned, icsr_if *> register_mapping_icsr;

	icsr_iprio_arr iprio;

	icsr_eidelivery eidelivery;
	icsr_eithreshold eithreshold;

	static constexpr unsigned eip_eie_arr_size = iss_config::IMSIC_MAX_IRQS / icsr_32::BITS_PER_CSR;
	static_assert(eip_eie_arr_size * icsr_32::BITS_PER_CSR == iss_config::IMSIC_MAX_IRQS);
	static_assert(iss_config::IMSIC_MAX_IRQS % icsr_32::BITS_PER_CSR == 0);

	icsr_32 eip[eip_eie_arr_size];
	icsr_32 eie[eip_eie_arr_size];

	// SMPU extension
	icsr_smpuaddr smpuaddr[SMPU_NREGIONS];
	icsr_smpuconf smpuconf[SMPU_NREGIONS];

	// HSMPU extension
	icsr_smpuaddr hmpuaddr[SMPU_NREGIONS];
	icsr_smpuconf hmpuconf[SMPU_NREGIONS];

	icsr_ms_table(PrivilegeLevel level) {
		assert(level == MachineMode || level == SupervisorMode);

		using namespace icsr;

		//    Major interrupt number
		//    |       interrupt name
		//    |       |
		//    v       v
		//    11      EXC_M_EXTERNAL_INTERRUPT
		//    3       EXC_M_SOFTWARE_INTERRUPT
		//    7       EXC_M_TIMER_INTERRUPT
		//    9       EXC_S_EXTERNAL_INTERRUPT
		//    1       EXC_S_SOFTWARE_INTERRUPT
		//    5       EXC_S_TIMER_INTERRUPT
		//    12      EXC_S_GUEST_EXTERNAL_INTERRUPT
		//    10      EXC_VS_EXTERNAL_INTERRUPT
		//    2       EXC_VS_SOFTWARE_INTERRUPT
		//    6       EXC_VS_TIMER_INTERRUPT

		iprio.map(register_mapping_icsr);

		if (level == MachineMode) {
			iprio.mark_present_in_hw(EXC_S_SOFTWARE_INTERRUPT);
			iprio.mark_present_in_hw(EXC_M_SOFTWARE_INTERRUPT);
			iprio.mark_present_in_hw(EXC_S_TIMER_INTERRUPT);
			iprio.mark_present_in_hw(EXC_M_TIMER_INTERRUPT);
			iprio.mark_present_in_hw(EXC_S_EXTERNAL_INTERRUPT);

			iprio.mark_present_in_hw(EXC_COUNTER_OVREFLOW_INTERRUPT);
			iprio.mark_present_in_hw(EXC_DEBUG_TRACE_INTERRUPT);
			iprio.mark_present_in_hw(EXC_LOW_PRIO_RAS_INTERRUPT);
			iprio.mark_present_in_hw(EXC_HIGH_PRIO_RAS_INTERRUPT);
			iprio.mark_present_in_hw(EXC_WDT_INTERRUPT);
		} else {
			iprio.mark_present_in_hw(EXC_S_SOFTWARE_INTERRUPT);
			iprio.mark_present_in_hw(EXC_VS_SOFTWARE_INTERRUPT);
			iprio.mark_present_in_hw(EXC_S_TIMER_INTERRUPT);
			iprio.mark_present_in_hw(EXC_VS_TIMER_INTERRUPT);
			iprio.mark_present_in_hw(EXC_VS_EXTERNAL_INTERRUPT);
			iprio.mark_present_in_hw(EXC_S_GUEST_EXTERNAL_INTERRUPT);

			iprio.mark_present_in_hw(EXC_COUNTER_OVREFLOW_INTERRUPT);
			iprio.mark_present_in_hw(EXC_DEBUG_TRACE_INTERRUPT);
			iprio.mark_present_in_hw(EXC_LOW_PRIO_RAS_INTERRUPT);
			iprio.mark_present_in_hw(EXC_HIGH_PRIO_RAS_INTERRUPT);
			iprio.mark_present_in_hw(EXC_WDT_INTERRUPT);
		}

		register_mapping_icsr[icsr_addr_eidelivery] = &eidelivery;
		register_mapping_icsr[icsr_addr_eithreshold] = &eithreshold;

		eip[0].mask = 0xFFFFFFFE; // IID 0 is not supported
		for (unsigned i = 0; i < eip_eie_arr_size; ++i)
			register_mapping_icsr[icsr_addr_eip0 + i] = &eip[i];

		eie[0].mask = 0xFFFFFFFE; // IID 0 is not supported
		for (unsigned i = 0; i < eip_eie_arr_size; ++i)
			register_mapping_icsr[icsr_addr_eie0 + i] = &eie[i];

		if (level == SupervisorMode) {
			for (unsigned i = 0; i < SMPU_NREGIONS; ++i) {
				register_mapping_icsr[icsr_addr_smpuaddr0 + 2 * i] = &smpuaddr[i];
				register_mapping_icsr[icsr_addr_smpuconf0 + 2 * i] = &smpuconf[i];
			}

			for (unsigned i = 0; i < SMPU_NREGIONS; ++i) {
				register_mapping_icsr[icsr_addr_hmpuaddr0 + 2 * i] = &hmpuaddr[i];
				register_mapping_icsr[icsr_addr_hmpuconf0 + 2 * i] = &hmpuconf[i];
			}
		}
	}

	bool is_valid_addr(unsigned addr) {
		return register_mapping_icsr.find(addr) != register_mapping_icsr.end();
	}

	void default_write32(unsigned addr, uint32_t value) {
		auto it = register_mapping_icsr.find(addr);
		ensure((it != register_mapping_icsr.end()) && "validate address before calling this function");
		it->second->checked_write(value);
	}

	uint32_t default_read32(unsigned addr) {
		auto it = register_mapping_icsr.find(addr);
		ensure((it != register_mapping_icsr.end()) && "validate address before calling this function");
		return it->second->checked_read();
	}
};


struct icsr_vs_table {
	static constexpr unsigned eip_eie_arr_size = iss_config::IMSIC_MAX_IRQS / icsr_32::BITS_PER_CSR;
	static_assert(iss_config::IMSIC_MAX_IRQS % icsr_32::BITS_PER_CSR == 0);

	static_assert(eip_eie_arr_size * 32 == iss_config::IMSIC_MAX_IRQS);

	vs_iprio_banks iprio;

	struct bank {
		icsr_eidelivery eidelivery;
		icsr_eithreshold eithreshold;

		icsr_32 eip[eip_eie_arr_size];
		icsr_32 eie[eip_eie_arr_size];

		// SMPU extension
		icsr_smpuaddr smpuaddr[SMPU_NREGIONS];
		icsr_smpuconf smpuconf[SMPU_NREGIONS];
	} bank[iss_config::MAX_GUEST];

	std::unordered_map<unsigned, icsr_if *> register_mapping_icsr[csr_hstatus::MAX_VGEIN_BANKS];

	icsr_vs_table() {
		using namespace icsr;

		iprio.map(register_mapping_icsr);

		for (unsigned i = csr_hstatus::VGEIN_FIRST_GUEST; i < csr_hstatus::MAX_VGEIN_BANKS; i++) {
			register_mapping_icsr[i][icsr_addr_eidelivery] = &get_guest_bank(i).eidelivery;
			register_mapping_icsr[i][icsr_addr_eithreshold] = &get_guest_bank(i).eithreshold;

			get_guest_bank(i).eip[0].mask = 0xFFFFFFFE; // IID 0 is not supported
			for (unsigned k = 0; k < eip_eie_arr_size; k++)
				register_mapping_icsr[i][icsr_addr_eip0 + k] = &get_guest_bank(i).eip[k];

			get_guest_bank(i).eie[0].mask = 0xFFFFFFFE; // IID 0 is not supported
			for (unsigned k = 0; k < eip_eie_arr_size; k++)
				register_mapping_icsr[i][icsr_addr_eie0 + k] = &get_guest_bank(i).eie[k];

			for (unsigned k = 0; k < SMPU_NREGIONS; ++k) {
				register_mapping_icsr[i][icsr_addr_smpuaddr0 + 2 * k] = &get_guest_bank(i).smpuaddr[k];
				register_mapping_icsr[i][icsr_addr_smpuconf0 + 2 * k] = &get_guest_bank(i).smpuconf[k];
			}
		}
	}

	bool is_valid_addr(unsigned addr, uint32_t vgein) {
		assert(vgein <= iss_config::MAX_GUEST);
		return register_mapping_icsr[vgein].find(addr) != register_mapping_icsr[vgein].end();
	}

	void default_write32(unsigned addr, unsigned vgein, uint32_t value) {
		assert(vgein <= iss_config::MAX_GUEST);
		auto it = register_mapping_icsr[vgein].find(addr);
		ensure((it != register_mapping_icsr[vgein].end()) && "validate address before calling this function");
		it->second->checked_write(value);
	}

	uint32_t default_read32(unsigned addr, unsigned vgein) {
		assert(vgein <= iss_config::MAX_GUEST);
		auto it = register_mapping_icsr[vgein].find(addr);
		ensure((it != register_mapping_icsr[vgein].end()) && "validate address before calling this function");
		return it->second->checked_read();
	}

	struct bank & get_guest_bank(uint32_t vgein) {
		assert(vgein > 0 && vgein <= iss_config::MAX_GUEST);

		return bank[vgein_to_id(vgein)];
	}
};


#define SWITCH_CASE_MATCH_ANY_HPMCOUNTER_RV32 \
	case HPMCOUNTER3_ADDR:                    \
	case HPMCOUNTER4_ADDR:                    \
	case HPMCOUNTER5_ADDR:                    \
	case HPMCOUNTER6_ADDR:                    \
	case HPMCOUNTER7_ADDR:                    \
	case HPMCOUNTER8_ADDR:                    \
	case HPMCOUNTER9_ADDR:                    \
	case HPMCOUNTER10_ADDR:                   \
	case HPMCOUNTER11_ADDR:                   \
	case HPMCOUNTER12_ADDR:                   \
	case HPMCOUNTER13_ADDR:                   \
	case HPMCOUNTER14_ADDR:                   \
	case HPMCOUNTER15_ADDR:                   \
	case HPMCOUNTER16_ADDR:                   \
	case HPMCOUNTER17_ADDR:                   \
	case HPMCOUNTER18_ADDR:                   \
	case HPMCOUNTER19_ADDR:                   \
	case HPMCOUNTER20_ADDR:                   \
	case HPMCOUNTER21_ADDR:                   \
	case HPMCOUNTER22_ADDR:                   \
	case HPMCOUNTER23_ADDR:                   \
	case HPMCOUNTER24_ADDR:                   \
	case HPMCOUNTER25_ADDR:                   \
	case HPMCOUNTER26_ADDR:                   \
	case HPMCOUNTER27_ADDR:                   \
	case HPMCOUNTER28_ADDR:                   \
	case HPMCOUNTER29_ADDR:                   \
	case HPMCOUNTER30_ADDR:                   \
	case HPMCOUNTER31_ADDR:                   \
	case HPMCOUNTER3H_ADDR:                   \
	case HPMCOUNTER4H_ADDR:                   \
	case HPMCOUNTER5H_ADDR:                   \
	case HPMCOUNTER6H_ADDR:                   \
	case HPMCOUNTER7H_ADDR:                   \
	case HPMCOUNTER8H_ADDR:                   \
	case HPMCOUNTER9H_ADDR:                   \
	case HPMCOUNTER10H_ADDR:                  \
	case HPMCOUNTER11H_ADDR:                  \
	case HPMCOUNTER12H_ADDR:                  \
	case HPMCOUNTER13H_ADDR:                  \
	case HPMCOUNTER14H_ADDR:                  \
	case HPMCOUNTER15H_ADDR:                  \
	case HPMCOUNTER16H_ADDR:                  \
	case HPMCOUNTER17H_ADDR:                  \
	case HPMCOUNTER18H_ADDR:                  \
	case HPMCOUNTER19H_ADDR:                  \
	case HPMCOUNTER20H_ADDR:                  \
	case HPMCOUNTER21H_ADDR:                  \
	case HPMCOUNTER22H_ADDR:                  \
	case HPMCOUNTER23H_ADDR:                  \
	case HPMCOUNTER24H_ADDR:                  \
	case HPMCOUNTER25H_ADDR:                  \
	case HPMCOUNTER26H_ADDR:                  \
	case HPMCOUNTER27H_ADDR:                  \
	case HPMCOUNTER28H_ADDR:                  \
	case HPMCOUNTER29H_ADDR:                  \
	case HPMCOUNTER30H_ADDR:                  \
	case HPMCOUNTER31H_ADDR:                  \
	case MHPMCOUNTER3_ADDR:                   \
	case MHPMCOUNTER4_ADDR:                   \
	case MHPMCOUNTER5_ADDR:                   \
	case MHPMCOUNTER6_ADDR:                   \
	case MHPMCOUNTER7_ADDR:                   \
	case MHPMCOUNTER8_ADDR:                   \
	case MHPMCOUNTER9_ADDR:                   \
	case MHPMCOUNTER10_ADDR:                  \
	case MHPMCOUNTER11_ADDR:                  \
	case MHPMCOUNTER12_ADDR:                  \
	case MHPMCOUNTER13_ADDR:                  \
	case MHPMCOUNTER14_ADDR:                  \
	case MHPMCOUNTER15_ADDR:                  \
	case MHPMCOUNTER16_ADDR:                  \
	case MHPMCOUNTER17_ADDR:                  \
	case MHPMCOUNTER18_ADDR:                  \
	case MHPMCOUNTER19_ADDR:                  \
	case MHPMCOUNTER20_ADDR:                  \
	case MHPMCOUNTER21_ADDR:                  \
	case MHPMCOUNTER22_ADDR:                  \
	case MHPMCOUNTER23_ADDR:                  \
	case MHPMCOUNTER24_ADDR:                  \
	case MHPMCOUNTER25_ADDR:                  \
	case MHPMCOUNTER26_ADDR:                  \
	case MHPMCOUNTER27_ADDR:                  \
	case MHPMCOUNTER28_ADDR:                  \
	case MHPMCOUNTER29_ADDR:                  \
	case MHPMCOUNTER30_ADDR:                  \
	case MHPMCOUNTER31_ADDR:                  \
	case MHPMCOUNTER3H_ADDR:                  \
	case MHPMCOUNTER4H_ADDR:                  \
	case MHPMCOUNTER5H_ADDR:                  \
	case MHPMCOUNTER6H_ADDR:                  \
	case MHPMCOUNTER7H_ADDR:                  \
	case MHPMCOUNTER8H_ADDR:                  \
	case MHPMCOUNTER9H_ADDR:                  \
	case MHPMCOUNTER10H_ADDR:                 \
	case MHPMCOUNTER11H_ADDR:                 \
	case MHPMCOUNTER12H_ADDR:                 \
	case MHPMCOUNTER13H_ADDR:                 \
	case MHPMCOUNTER14H_ADDR:                 \
	case MHPMCOUNTER15H_ADDR:                 \
	case MHPMCOUNTER16H_ADDR:                 \
	case MHPMCOUNTER17H_ADDR:                 \
	case MHPMCOUNTER18H_ADDR:                 \
	case MHPMCOUNTER19H_ADDR:                 \
	case MHPMCOUNTER20H_ADDR:                 \
	case MHPMCOUNTER21H_ADDR:                 \
	case MHPMCOUNTER22H_ADDR:                 \
	case MHPMCOUNTER23H_ADDR:                 \
	case MHPMCOUNTER24H_ADDR:                 \
	case MHPMCOUNTER25H_ADDR:                 \
	case MHPMCOUNTER26H_ADDR:                 \
	case MHPMCOUNTER27H_ADDR:                 \
	case MHPMCOUNTER28H_ADDR:                 \
	case MHPMCOUNTER29H_ADDR:                 \
	case MHPMCOUNTER30H_ADDR:                 \
	case MHPMCOUNTER31H_ADDR:                 \
	case MHPMEVENT3_ADDR:                     \
	case MHPMEVENT4_ADDR:                     \
	case MHPMEVENT5_ADDR:                     \
	case MHPMEVENT6_ADDR:                     \
	case MHPMEVENT7_ADDR:                     \
	case MHPMEVENT8_ADDR:                     \
	case MHPMEVENT9_ADDR:                     \
	case MHPMEVENT10_ADDR:                    \
	case MHPMEVENT11_ADDR:                    \
	case MHPMEVENT12_ADDR:                    \
	case MHPMEVENT13_ADDR:                    \
	case MHPMEVENT14_ADDR:                    \
	case MHPMEVENT15_ADDR:                    \
	case MHPMEVENT16_ADDR:                    \
	case MHPMEVENT17_ADDR:                    \
	case MHPMEVENT18_ADDR:                    \
	case MHPMEVENT19_ADDR:                    \
	case MHPMEVENT20_ADDR:                    \
	case MHPMEVENT21_ADDR:                    \
	case MHPMEVENT22_ADDR:                    \
	case MHPMEVENT23_ADDR:                    \
	case MHPMEVENT24_ADDR:                    \
	case MHPMEVENT25_ADDR:                    \
	case MHPMEVENT26_ADDR:                    \
	case MHPMEVENT27_ADDR:                    \
	case MHPMEVENT28_ADDR:                    \
	case MHPMEVENT29_ADDR:                    \
	case MHPMEVENT30_ADDR:                    \
	case MHPMEVENT31_ADDR

}  // namespace rv32
