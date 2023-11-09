#pragma once

#include <stdint.h>
#include <unordered_map>

#include "csr.h"

namespace rv32 {

struct csr_name_mapping {
        std::unordered_map<unsigned, const char *> name_mapping;

        csr_name_mapping() {
                // NOTE: start of auto generated

                // user counters/timers
                add_csr_name_mapping("cycle", 0xC00);
                add_csr_name_mapping("time", 0xC01);
                add_csr_name_mapping("instret", 0xC02);
                add_csr_name_mapping("hpmcounter3", 0xC03);
                add_csr_name_mapping("hpmcounter4", 0xC04);
                add_csr_name_mapping("hpmcounter5", 0xC05);
                add_csr_name_mapping("hpmcounter6", 0xC06);
                add_csr_name_mapping("hpmcounter7", 0xC07);
                add_csr_name_mapping("hpmcounter8", 0xC08);
                add_csr_name_mapping("hpmcounter9", 0xC09);
                add_csr_name_mapping("hpmcounter10", 0xC0A);
                add_csr_name_mapping("hpmcounter11", 0xC0B);
                add_csr_name_mapping("hpmcounter12", 0xC0C);
                add_csr_name_mapping("hpmcounter13", 0xC0D);
                add_csr_name_mapping("hpmcounter14", 0xC0E);
                add_csr_name_mapping("hpmcounter15", 0xC0F);
                add_csr_name_mapping("hpmcounter16", 0xC10);
                add_csr_name_mapping("hpmcounter17", 0xC11);
                add_csr_name_mapping("hpmcounter18", 0xC12);
                add_csr_name_mapping("hpmcounter19", 0xC13);
                add_csr_name_mapping("hpmcounter20", 0xC14);
                add_csr_name_mapping("hpmcounter21", 0xC15);
                add_csr_name_mapping("hpmcounter22", 0xC16);
                add_csr_name_mapping("hpmcounter23", 0xC17);
                add_csr_name_mapping("hpmcounter24", 0xC18);
                add_csr_name_mapping("hpmcounter25", 0xC19);
                add_csr_name_mapping("hpmcounter26", 0xC1A);
                add_csr_name_mapping("hpmcounter27", 0xC1B);
                add_csr_name_mapping("hpmcounter28", 0xC1C);
                add_csr_name_mapping("hpmcounter29", 0xC1D);
                add_csr_name_mapping("hpmcounter30", 0xC1E);
                add_csr_name_mapping("hpmcounter31", 0xC1F);
                add_csr_name_mapping("cycleh", 0xC80);
                add_csr_name_mapping("timeh", 0xC81);
                add_csr_name_mapping("instreth", 0xC82);
                add_csr_name_mapping("hpmcounter3h", 0xC83);
                add_csr_name_mapping("hpmcounter4h", 0xC84);
                add_csr_name_mapping("hpmcounter5h", 0xC85);
                add_csr_name_mapping("hpmcounter6h", 0xC86);
                add_csr_name_mapping("hpmcounter7h", 0xC87);
                add_csr_name_mapping("hpmcounter8h", 0xC88);
                add_csr_name_mapping("hpmcounter9h", 0xC89);
                add_csr_name_mapping("hpmcounter10h", 0xC8A);
                add_csr_name_mapping("hpmcounter11h", 0xC8B);
                add_csr_name_mapping("hpmcounter12h", 0xC8C);
                add_csr_name_mapping("hpmcounter13h", 0xC8D);
                add_csr_name_mapping("hpmcounter14h", 0xC8E);
                add_csr_name_mapping("hpmcounter15h", 0xC8F);
                add_csr_name_mapping("hpmcounter16h", 0xC90);
                add_csr_name_mapping("hpmcounter17h", 0xC91);
                add_csr_name_mapping("hpmcounter18h", 0xC92);
                add_csr_name_mapping("hpmcounter19h", 0xC93);
                add_csr_name_mapping("hpmcounter20h", 0xC94);
                add_csr_name_mapping("hpmcounter21h", 0xC95);
                add_csr_name_mapping("hpmcounter22h", 0xC96);
                add_csr_name_mapping("hpmcounter23h", 0xC97);
                add_csr_name_mapping("hpmcounter24h", 0xC98);
                add_csr_name_mapping("hpmcounter25h", 0xC99);
                add_csr_name_mapping("hpmcounter26h", 0xC9A);
                add_csr_name_mapping("hpmcounter27h", 0xC9B);
                add_csr_name_mapping("hpmcounter28h", 0xC9C);
                add_csr_name_mapping("hpmcounter29h", 0xC9D);
                add_csr_name_mapping("hpmcounter30h", 0xC9E);
                add_csr_name_mapping("hpmcounter31h", 0xC9F);
                // supervisor
                add_csr_name_mapping("sstatus", 0x100);
                add_csr_name_mapping("sie", 0x104);
                add_csr_name_mapping("stvec", 0x105);
                add_csr_name_mapping("scounteren", 0x106);
                add_csr_name_mapping("senvcfg", 0x10A);
                add_csr_name_mapping("sscratch", 0x140);
                add_csr_name_mapping("sepc", 0x141);
                add_csr_name_mapping("scause", 0x142);
                add_csr_name_mapping("stval", 0x143);
                add_csr_name_mapping("sip", 0x144);
                add_csr_name_mapping("satp", 0x180);
                // machine
                add_csr_name_mapping("mvendorid", 0xF11);
                add_csr_name_mapping("marchid", 0xF12);
                add_csr_name_mapping("mimpid", 0xF13);
                add_csr_name_mapping("mhartid", 0xF14);
                add_csr_name_mapping("mconfigptr", 0xF15);
                add_csr_name_mapping("mstatus", 0x300);
                add_csr_name_mapping("misa", 0x301);
                add_csr_name_mapping("medeleg", 0x302);
                add_csr_name_mapping("mideleg", 0x303);
                add_csr_name_mapping("mie", 0x304);
                add_csr_name_mapping("mtvec", 0x305);
                add_csr_name_mapping("mcounteren", 0x306);
                add_csr_name_mapping("mstatush", 0x310);
                add_csr_name_mapping("mscratch", 0x340);
                add_csr_name_mapping("mepc", 0x341);
                add_csr_name_mapping("mcause", 0x342);
                add_csr_name_mapping("mtval", 0x343);
                add_csr_name_mapping("mip", 0x344);
                add_csr_name_mapping("mtinst", 0x34A);
                add_csr_name_mapping("mtval2", 0x34B);
                add_csr_name_mapping("menvcfg", 0x30A);
                add_csr_name_mapping("menvcfgh", 0x31A);
                add_csr_name_mapping("mseccfg", 0x747);
                add_csr_name_mapping("mseccfgh", 0x757);
                add_csr_name_mapping("pmpcfg0", 0x3A0);
                add_csr_name_mapping("pmpcfg1", 0x3A1);
                add_csr_name_mapping("pmpcfg2", 0x3A2);
                add_csr_name_mapping("pmpcfg3", 0x3A3);
                add_csr_name_mapping("pmpcfg4", 0x3A4);
                add_csr_name_mapping("pmpcfg5", 0x3A5);
                add_csr_name_mapping("pmpcfg6", 0x3A6);
                add_csr_name_mapping("pmpcfg7", 0x3A7);
                add_csr_name_mapping("pmpcfg8", 0x3A8);
                add_csr_name_mapping("pmpcfg9", 0x3A9);
                add_csr_name_mapping("pmpcfg10", 0x3AA);
                add_csr_name_mapping("pmpcfg11", 0x3AB);
                add_csr_name_mapping("pmpcfg12", 0x3AC);
                add_csr_name_mapping("pmpcfg13", 0x3AD);
                add_csr_name_mapping("pmpcfg14", 0x3AE);
                add_csr_name_mapping("pmpcfg15", 0x3AF);
                add_csr_name_mapping("pmpaddr0", 0x3B0);
                add_csr_name_mapping("pmpaddr1", 0x3B1);
                add_csr_name_mapping("pmpaddr2", 0x3B2);
                add_csr_name_mapping("pmpaddr3", 0x3B3);
                add_csr_name_mapping("pmpaddr4", 0x3B4);
                add_csr_name_mapping("pmpaddr5", 0x3B5);
                add_csr_name_mapping("pmpaddr6", 0x3B6);
                add_csr_name_mapping("pmpaddr7", 0x3B7);
                add_csr_name_mapping("pmpaddr8", 0x3B8);
                add_csr_name_mapping("pmpaddr9", 0x3B9);
                add_csr_name_mapping("pmpaddr10", 0x3BA);
                add_csr_name_mapping("pmpaddr11", 0x3BB);
                add_csr_name_mapping("pmpaddr12", 0x3BC);
                add_csr_name_mapping("pmpaddr13", 0x3BD);
                add_csr_name_mapping("pmpaddr14", 0x3BE);
                add_csr_name_mapping("pmpaddr15", 0x3BF);
                add_csr_name_mapping("pmpaddr16", 0x3C0);
                add_csr_name_mapping("pmpaddr17", 0x3C1);
                add_csr_name_mapping("pmpaddr18", 0x3C2);
                add_csr_name_mapping("pmpaddr19", 0x3C3);
                add_csr_name_mapping("pmpaddr20", 0x3C4);
                add_csr_name_mapping("pmpaddr21", 0x3C5);
                add_csr_name_mapping("pmpaddr22", 0x3C6);
                add_csr_name_mapping("pmpaddr23", 0x3C7);
                add_csr_name_mapping("pmpaddr24", 0x3C8);
                add_csr_name_mapping("pmpaddr25", 0x3C9);
                add_csr_name_mapping("pmpaddr26", 0x3CA);
                add_csr_name_mapping("pmpaddr27", 0x3CB);
                add_csr_name_mapping("pmpaddr28", 0x3CC);
                add_csr_name_mapping("pmpaddr29", 0x3CD);
                add_csr_name_mapping("pmpaddr30", 0x3CE);
                add_csr_name_mapping("pmpaddr31", 0x3CF);
                add_csr_name_mapping("pmpaddr32", 0x3D0);
                add_csr_name_mapping("pmpaddr33", 0x3D1);
                add_csr_name_mapping("pmpaddr34", 0x3D2);
                add_csr_name_mapping("pmpaddr35", 0x3D3);
                add_csr_name_mapping("pmpaddr36", 0x3D4);
                add_csr_name_mapping("pmpaddr37", 0x3D5);
                add_csr_name_mapping("pmpaddr38", 0x3D6);
                add_csr_name_mapping("pmpaddr39", 0x3D7);
                add_csr_name_mapping("pmpaddr40", 0x3D8);
                add_csr_name_mapping("pmpaddr41", 0x3D9);
                add_csr_name_mapping("pmpaddr42", 0x3DA);
                add_csr_name_mapping("pmpaddr43", 0x3DB);
                add_csr_name_mapping("pmpaddr44", 0x3DC);
                add_csr_name_mapping("pmpaddr45", 0x3DD);
                add_csr_name_mapping("pmpaddr46", 0x3DE);
                add_csr_name_mapping("pmpaddr47", 0x3DF);
                add_csr_name_mapping("pmpaddr48", 0x3E0);
                add_csr_name_mapping("pmpaddr49", 0x3E1);
                add_csr_name_mapping("pmpaddr50", 0x3E2);
                add_csr_name_mapping("pmpaddr51", 0x3E3);
                add_csr_name_mapping("pmpaddr52", 0x3E4);
                add_csr_name_mapping("pmpaddr53", 0x3E5);
                add_csr_name_mapping("pmpaddr54", 0x3E6);
                add_csr_name_mapping("pmpaddr55", 0x3E7);
                add_csr_name_mapping("pmpaddr56", 0x3E8);
                add_csr_name_mapping("pmpaddr57", 0x3E9);
                add_csr_name_mapping("pmpaddr58", 0x3EA);
                add_csr_name_mapping("pmpaddr59", 0x3EB);
                add_csr_name_mapping("pmpaddr60", 0x3EC);
                add_csr_name_mapping("pmpaddr61", 0x3ED);
                add_csr_name_mapping("pmpaddr62", 0x3EE);
                add_csr_name_mapping("pmpaddr63", 0x3EF);
                add_csr_name_mapping("mcycle", 0xB00);
                add_csr_name_mapping("minstret", 0xB02);
                add_csr_name_mapping("mhpmcounter3", 0xB03);
                add_csr_name_mapping("mhpmcounter4", 0xB04);
                add_csr_name_mapping("mhpmcounter5", 0xB05);
                add_csr_name_mapping("mhpmcounter6", 0xB06);
                add_csr_name_mapping("mhpmcounter7", 0xB07);
                add_csr_name_mapping("mhpmcounter8", 0xB08);
                add_csr_name_mapping("mhpmcounter9", 0xB09);
                add_csr_name_mapping("mhpmcounter10", 0xB0A);
                add_csr_name_mapping("mhpmcounter11", 0xB0B);
                add_csr_name_mapping("mhpmcounter12", 0xB0C);
                add_csr_name_mapping("mhpmcounter13", 0xB0D);
                add_csr_name_mapping("mhpmcounter14", 0xB0E);
                add_csr_name_mapping("mhpmcounter15", 0xB0F);
                add_csr_name_mapping("mhpmcounter16", 0xB10);
                add_csr_name_mapping("mhpmcounter17", 0xB11);
                add_csr_name_mapping("mhpmcounter18", 0xB12);
                add_csr_name_mapping("mhpmcounter19", 0xB13);
                add_csr_name_mapping("mhpmcounter20", 0xB14);
                add_csr_name_mapping("mhpmcounter21", 0xB15);
                add_csr_name_mapping("mhpmcounter22", 0xB16);
                add_csr_name_mapping("mhpmcounter23", 0xB17);
                add_csr_name_mapping("mhpmcounter24", 0xB18);
                add_csr_name_mapping("mhpmcounter25", 0xB19);
                add_csr_name_mapping("mhpmcounter26", 0xB1A);
                add_csr_name_mapping("mhpmcounter27", 0xB1B);
                add_csr_name_mapping("mhpmcounter28", 0xB1C);
                add_csr_name_mapping("mhpmcounter29", 0xB1D);
                add_csr_name_mapping("mhpmcounter30", 0xB1E);
                add_csr_name_mapping("mhpmcounter31", 0xB1F);
                add_csr_name_mapping("mcycleh", 0xB80);
                add_csr_name_mapping("minstreth", 0xB82);
                add_csr_name_mapping("mhpmcounter3h", 0xB83);
                add_csr_name_mapping("mhpmcounter4h", 0xB84);
                add_csr_name_mapping("mhpmcounter5h", 0xB85);
                add_csr_name_mapping("mhpmcounter6h", 0xB86);
                add_csr_name_mapping("mhpmcounter7h", 0xB87);
                add_csr_name_mapping("mhpmcounter8h", 0xB88);
                add_csr_name_mapping("mhpmcounter9h", 0xB89);
                add_csr_name_mapping("mhpmcounter10h", 0xB8A);
                add_csr_name_mapping("mhpmcounter11h", 0xB8B);
                add_csr_name_mapping("mhpmcounter12h", 0xB8C);
                add_csr_name_mapping("mhpmcounter13h", 0xB8D);
                add_csr_name_mapping("mhpmcounter14h", 0xB8E);
                add_csr_name_mapping("mhpmcounter15h", 0xB8F);
                add_csr_name_mapping("mhpmcounter16h", 0xB90);
                add_csr_name_mapping("mhpmcounter17h", 0xB91);
                add_csr_name_mapping("mhpmcounter18h", 0xB92);
                add_csr_name_mapping("mhpmcounter19h", 0xB93);
                add_csr_name_mapping("mhpmcounter20h", 0xB94);
                add_csr_name_mapping("mhpmcounter21h", 0xB95);
                add_csr_name_mapping("mhpmcounter22h", 0xB96);
                add_csr_name_mapping("mhpmcounter23h", 0xB97);
                add_csr_name_mapping("mhpmcounter24h", 0xB98);
                add_csr_name_mapping("mhpmcounter25h", 0xB99);
                add_csr_name_mapping("mhpmcounter26h", 0xB9A);
                add_csr_name_mapping("mhpmcounter27h", 0xB9B);
                add_csr_name_mapping("mhpmcounter28h", 0xB9C);
                add_csr_name_mapping("mhpmcounter29h", 0xB9D);
                add_csr_name_mapping("mhpmcounter30h", 0xB9E);
                add_csr_name_mapping("mhpmcounter31h", 0xB9F);
                add_csr_name_mapping("mcountinhibit", 0x320);
                add_csr_name_mapping("mhpmevent3", 0x323);
                add_csr_name_mapping("mhpmevent4", 0x324);
                add_csr_name_mapping("mhpmevent5", 0x325);
                add_csr_name_mapping("mhpmevent6", 0x326);
                add_csr_name_mapping("mhpmevent7", 0x327);
                add_csr_name_mapping("mhpmevent8", 0x328);
                add_csr_name_mapping("mhpmevent9", 0x329);
                add_csr_name_mapping("mhpmevent10", 0x32A);
                add_csr_name_mapping("mhpmevent11", 0x32B);
                add_csr_name_mapping("mhpmevent12", 0x32C);
                add_csr_name_mapping("mhpmevent13", 0x32D);
                add_csr_name_mapping("mhpmevent14", 0x32E);
                add_csr_name_mapping("mhpmevent15", 0x32F);
                add_csr_name_mapping("mhpmevent16", 0x330);
                add_csr_name_mapping("mhpmevent17", 0x331);
                add_csr_name_mapping("mhpmevent18", 0x332);
                add_csr_name_mapping("mhpmevent19", 0x333);
                add_csr_name_mapping("mhpmevent20", 0x334);
                add_csr_name_mapping("mhpmevent21", 0x335);
                add_csr_name_mapping("mhpmevent22", 0x336);
                add_csr_name_mapping("mhpmevent23", 0x337);
                add_csr_name_mapping("mhpmevent24", 0x338);
                add_csr_name_mapping("mhpmevent25", 0x339);
                add_csr_name_mapping("mhpmevent26", 0x33A);
                add_csr_name_mapping("mhpmevent27", 0x33B);
                add_csr_name_mapping("mhpmevent28", 0x33C);
                add_csr_name_mapping("mhpmevent29", 0x33D);
                add_csr_name_mapping("mhpmevent30", 0x33E);
                add_csr_name_mapping("mhpmevent31", 0x33F);
                // hypervisor
                add_csr_name_mapping("hstatus", 0x600);
                add_csr_name_mapping("hedeleg", 0x602);
                add_csr_name_mapping("hideleg", 0x603);
                add_csr_name_mapping("hie", 0x604);
                add_csr_name_mapping("hcounteren", 0x606);
                add_csr_name_mapping("hgeie", 0x607);
                add_csr_name_mapping("htval", 0x643);
                add_csr_name_mapping("hip", 0x644);
                add_csr_name_mapping("hvip", 0x645);
                add_csr_name_mapping("htinst", 0x64A);
                add_csr_name_mapping("hgeip", 0xE12);
                add_csr_name_mapping("henvcfg", 0x60A);
                add_csr_name_mapping("henvcfgh", 0x61A);
                add_csr_name_mapping("hgatp", 0x680);
                add_csr_name_mapping("htimedelta", 0x605);
                add_csr_name_mapping("htimedeltah", 0x615);
                add_csr_name_mapping("vsstatus", 0x200);
                add_csr_name_mapping("vsie", 0x204);
                add_csr_name_mapping("vstvec", 0x205);
                add_csr_name_mapping("vsscratch", 0x240);
                add_csr_name_mapping("vsepc", 0x241);
                add_csr_name_mapping("vscause", 0x242);
                add_csr_name_mapping("vstval", 0x243);
                add_csr_name_mapping("vsip", 0x244);
                add_csr_name_mapping("vsatp", 0x280);
                // Smaia extension
                add_csr_name_mapping("miselect", 0x350);
                add_csr_name_mapping("mireg", 0x351);
                add_csr_name_mapping("mtopei", 0x35C);
                add_csr_name_mapping("mtopi", 0xFB0);
                add_csr_name_mapping("mvien", 0x308);
                add_csr_name_mapping("mvip", 0x309);
                add_csr_name_mapping("midelegh", 0x313);
                add_csr_name_mapping("mieh", 0x314);
                add_csr_name_mapping("mvienh", 0x318);
                add_csr_name_mapping("mviph", 0x319);
                add_csr_name_mapping("miph", 0x354);
                // Smcntrpmf extension
                add_csr_name_mapping("mcyclecfg", 0x321);
                add_csr_name_mapping("minstretcfg", 0x322);
                add_csr_name_mapping("mcyclecfgh", 0x721);
                add_csr_name_mapping("minstretcfgh", 0x722);
                // Smstateen extension
                add_csr_name_mapping("mstateen0", 0x30C);
                add_csr_name_mapping("mstateen1", 0x30D);
                add_csr_name_mapping("mstateen2", 0x30E);
                add_csr_name_mapping("mstateen3", 0x30F);
                add_csr_name_mapping("sstateen0", 0x10C);
                add_csr_name_mapping("sstateen1", 0x10D);
                add_csr_name_mapping("sstateen2", 0x10E);
                add_csr_name_mapping("sstateen3", 0x10F);
                add_csr_name_mapping("hstateen0", 0x60C);
                add_csr_name_mapping("hstateen1", 0x60D);
                add_csr_name_mapping("hstateen2", 0x60E);
                add_csr_name_mapping("hstateen3", 0x60F);
                add_csr_name_mapping("mstateen0h", 0x31C);
                add_csr_name_mapping("mstateen1h", 0x31D);
                add_csr_name_mapping("mstateen2h", 0x31E);
                add_csr_name_mapping("mstateen3h", 0x31F);
                add_csr_name_mapping("hstateen0h", 0x61C);
                add_csr_name_mapping("hstateen1h", 0x61D);
                add_csr_name_mapping("hstateen2h", 0x61E);
                add_csr_name_mapping("hstateen3h", 0x61F);
                // Ssaia extension
                add_csr_name_mapping("siselect", 0x150);
                add_csr_name_mapping("sireg", 0x151);
                add_csr_name_mapping("stopei", 0x15C);
                add_csr_name_mapping("stopi", 0xDB0);
                add_csr_name_mapping("sieh", 0x114);
                add_csr_name_mapping("siph", 0x154);
                add_csr_name_mapping("hvien", 0x608);
                add_csr_name_mapping("hvictl", 0x609);
                add_csr_name_mapping("hviprio1", 0x646);
                add_csr_name_mapping("hviprio2", 0x647);
                add_csr_name_mapping("vsiselect", 0x250);
                add_csr_name_mapping("vsireg", 0x251);
                add_csr_name_mapping("vstopei", 0x25C);
                add_csr_name_mapping("vstopi", 0xEB0);
                add_csr_name_mapping("hidelegh", 0x613);
                add_csr_name_mapping("hvienh", 0x618);
                add_csr_name_mapping("hviph", 0x655);
                add_csr_name_mapping("hviprio1h", 0x656);
                add_csr_name_mapping("hviprio2h", 0x657);
                add_csr_name_mapping("vsieh", 0x214);
                add_csr_name_mapping("vsiph", 0x254);
                // Sscofpmf extension
                add_csr_name_mapping("scountovf", 0xDA0);
                add_csr_name_mapping("mhpmevent3h", 0x723);
                add_csr_name_mapping("mhpmevent4h", 0x724);
                add_csr_name_mapping("mhpmevent5h", 0x725);
                add_csr_name_mapping("mhpmevent6h", 0x726);
                add_csr_name_mapping("mhpmevent7h", 0x727);
                add_csr_name_mapping("mhpmevent8h", 0x728);
                add_csr_name_mapping("mhpmevent9h", 0x729);
                add_csr_name_mapping("mhpmevent10h", 0x72A);
                add_csr_name_mapping("mhpmevent11h", 0x72B);
                add_csr_name_mapping("mhpmevent12h", 0x72C);
                add_csr_name_mapping("mhpmevent13h", 0x72D);
                add_csr_name_mapping("mhpmevent14h", 0x72E);
                add_csr_name_mapping("mhpmevent15h", 0x72F);
                add_csr_name_mapping("mhpmevent16h", 0x730);
                add_csr_name_mapping("mhpmevent17h", 0x731);
                add_csr_name_mapping("mhpmevent18h", 0x732);
                add_csr_name_mapping("mhpmevent19h", 0x733);
                add_csr_name_mapping("mhpmevent20h", 0x734);
                add_csr_name_mapping("mhpmevent21h", 0x735);
                add_csr_name_mapping("mhpmevent22h", 0x736);
                add_csr_name_mapping("mhpmevent23h", 0x737);
                add_csr_name_mapping("mhpmevent24h", 0x738);
                add_csr_name_mapping("mhpmevent25h", 0x739);
                add_csr_name_mapping("mhpmevent26h", 0x73A);
                add_csr_name_mapping("mhpmevent27h", 0x73B);
                add_csr_name_mapping("mhpmevent28h", 0x73C);
                add_csr_name_mapping("mhpmevent29h", 0x73D);
                add_csr_name_mapping("mhpmevent30h", 0x73E);
                add_csr_name_mapping("mhpmevent31h", 0x73F);
                // Sstc extension
                add_csr_name_mapping("stimecmp", 0x14D);
                add_csr_name_mapping("stimecmph", 0x15D);
                add_csr_name_mapping("vstimecmp", 0x24D);
                add_csr_name_mapping("vstimecmph", 0x25D);
                // dropped
                add_csr_name_mapping("ubadaddr", 0x43);
                add_csr_name_mapping("sbadaddr", 0x143);
                add_csr_name_mapping("sptbr", 0x180);
                add_csr_name_mapping("mbadaddr", 0x343);
                add_csr_name_mapping("mucounteren", 0x320);
                add_csr_name_mapping("mscounteren", 0x321);
                add_csr_name_mapping("mhcounteren", 0x322);
                add_csr_name_mapping("mbase", 0x380);
                add_csr_name_mapping("mbound", 0x381);
                add_csr_name_mapping("mibase", 0x382);
                add_csr_name_mapping("mibound", 0x383);
                add_csr_name_mapping("mdbase", 0x384);
                add_csr_name_mapping("mdbound", 0x385);
                add_csr_name_mapping("ustatus", 0x0);
                add_csr_name_mapping("uie", 0x4);
                add_csr_name_mapping("utvec", 0x5);
                add_csr_name_mapping("uscratch", 0x40);
                add_csr_name_mapping("uepc", 0x41);
                add_csr_name_mapping("ucause", 0x42);
                add_csr_name_mapping("utval", 0x43);
                add_csr_name_mapping("uip", 0x44);
                add_csr_name_mapping("sedeleg", 0x102);
                add_csr_name_mapping("sideleg", 0x103);
                // unprivileged
                add_csr_name_mapping("fflags", 0x1);
                add_csr_name_mapping("frm", 0x2);
                add_csr_name_mapping("fcsr", 0x3);
                add_csr_name_mapping("dcsr", 0x7B0);
                add_csr_name_mapping("dpc", 0x7B1);
                add_csr_name_mapping("dscratch0", 0x7B2);
                add_csr_name_mapping("dscratch1", 0x7B3);
                add_csr_name_mapping("dscratch", 0x7B2);
                add_csr_name_mapping("tselect", 0x7A0);
                add_csr_name_mapping("tdata1", 0x7A1);
                add_csr_name_mapping("tdata2", 0x7A2);
                add_csr_name_mapping("tdata3", 0x7A3);
                add_csr_name_mapping("tinfo", 0x7A4);
                add_csr_name_mapping("tcontrol", 0x7A5);
                add_csr_name_mapping("hcontext", 0x6A8);
                add_csr_name_mapping("scontext", 0x5A8);
                add_csr_name_mapping("mcontext", 0x7A8);
                add_csr_name_mapping("mscontext", 0x7AA);
                add_csr_name_mapping("mcontrol", 0x7A1);
                add_csr_name_mapping("mcontrol6", 0x7A1);
                add_csr_name_mapping("icount", 0x7A1);
                add_csr_name_mapping("itrigger", 0x7A1);
                add_csr_name_mapping("etrigger", 0x7A1);
                add_csr_name_mapping("tmexttrigger", 0x7A1);
                add_csr_name_mapping("textra32", 0x7A3);
                add_csr_name_mapping("textra64", 0x7A3);
                add_csr_name_mapping("seed", 0x15);
                add_csr_name_mapping("vstart", 0x8);
                add_csr_name_mapping("vxsat", 0x9);
                add_csr_name_mapping("vxrm", 0xA);
                add_csr_name_mapping("vcsr", 0xF);
                add_csr_name_mapping("vl", 0xC20);
                add_csr_name_mapping("vtype", 0xC21);
                add_csr_name_mapping("vlenb", 0xC22);

                // NOTE: end of auto generated

                // SMPU extension
                add_csr_name_mapping("smpumask", 0x128); // ToDo: This address is not yet specified and may change in the future
                // HS MPU extension
                add_csr_name_mapping("hmpumask", 0x620); // ToDo: This address is not yet specified and may change in the future
                add_csr_name_mapping("vsmpumask", 0x260); // ToDo: This address is not yet specified and may change in the future
        }

        const char * get_csr_name(unsigned addr) {
                auto it = name_mapping.find(addr);

                if (it == name_mapping.end()) {
                        return "?";
                } else {
                        return it->second;
                }
        }

private:
        void add_csr_name_mapping(const char * name, unsigned addr) {
                name_mapping[addr] = name;
        }
};


struct icsr_name_mapping {
        std::unordered_map<unsigned, const char *> name_mapping;

        icsr_name_mapping() {
                using namespace rv32::icsr;

                add_icsr_name_mapping("eidelivery", icsr_addr_eidelivery);
                add_icsr_name_mapping("eithreshold", icsr_addr_eithreshold);

                add_icsr_name_mapping("eip[0]", icsr_addr_eip0 + 0);
                add_icsr_name_mapping("eip[1]", icsr_addr_eip0 + 1);
                add_icsr_name_mapping("eip[2]", icsr_addr_eip0 + 2);
                add_icsr_name_mapping("eip[3]", icsr_addr_eip0 + 3);
                add_icsr_name_mapping("eip[4]", icsr_addr_eip0 + 4);
                add_icsr_name_mapping("eip[5]", icsr_addr_eip0 + 5);
                add_icsr_name_mapping("eip[6]", icsr_addr_eip0 + 6);
                add_icsr_name_mapping("eip[7]", icsr_addr_eip0 + 7);

                add_icsr_name_mapping("eie[0]", icsr_addr_eie0 + 0);
                add_icsr_name_mapping("eie[1]", icsr_addr_eie0 + 1);
                add_icsr_name_mapping("eie[2]", icsr_addr_eie0 + 2);
                add_icsr_name_mapping("eie[3]", icsr_addr_eie0 + 3);
                add_icsr_name_mapping("eie[4]", icsr_addr_eie0 + 4);
                add_icsr_name_mapping("eie[5]", icsr_addr_eie0 + 5);
                add_icsr_name_mapping("eie[6]", icsr_addr_eie0 + 6);
                add_icsr_name_mapping("eie[7]", icsr_addr_eie0 + 7);

                add_icsr_name_mapping("iprio[0]", icsr_iprio_arr::icsr_addr_iprio0 + 0);
                add_icsr_name_mapping("iprio[1]", icsr_iprio_arr::icsr_addr_iprio0 + 1);
                add_icsr_name_mapping("iprio[2]", icsr_iprio_arr::icsr_addr_iprio0 + 2);
                add_icsr_name_mapping("iprio[3]", icsr_iprio_arr::icsr_addr_iprio0 + 3);
                add_icsr_name_mapping("iprio[4]", icsr_iprio_arr::icsr_addr_iprio0 + 4);
                add_icsr_name_mapping("iprio[5]", icsr_iprio_arr::icsr_addr_iprio0 + 5);
                add_icsr_name_mapping("iprio[6]", icsr_iprio_arr::icsr_addr_iprio0 + 6);
                add_icsr_name_mapping("iprio[7]", icsr_iprio_arr::icsr_addr_iprio0 + 7);
                add_icsr_name_mapping("iprio[8]", icsr_iprio_arr::icsr_addr_iprio0 + 8);
                add_icsr_name_mapping("iprio[9]", icsr_iprio_arr::icsr_addr_iprio0 + 9);
                add_icsr_name_mapping("iprio[10]", icsr_iprio_arr::icsr_addr_iprio0 + 10);
                add_icsr_name_mapping("iprio[11]", icsr_iprio_arr::icsr_addr_iprio0 + 11);
                add_icsr_name_mapping("iprio[12]", icsr_iprio_arr::icsr_addr_iprio0 + 12);
                add_icsr_name_mapping("iprio[13]", icsr_iprio_arr::icsr_addr_iprio0 + 13);
                add_icsr_name_mapping("iprio[14]", icsr_iprio_arr::icsr_addr_iprio0 + 14);
                add_icsr_name_mapping("iprio[15]", icsr_iprio_arr::icsr_addr_iprio0 + 15);
        }

        const char * get_icsr_name(unsigned addr) {
                auto it = name_mapping.find(addr);

                if (it == name_mapping.end()) {
                        return "???";
                } else {
                        return it->second;
                }
        }

private:
        void add_icsr_name_mapping(const char * name, unsigned addr) {
                name_mapping[addr] = name;
        }
};

}
