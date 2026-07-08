# RISC-V External Timers Module Specification (revision 1.0)

This document specifies an external timer component that provides programmable multi-channel timer functionality.

## Functional Requirements

- Shared clock source
- Configurable source clock divider (16 bits)
- Configurable number of channels (design-time, range 1...8)
- Configurable (per channel) timer limit interval (32 bits)
- Each channel counts divided source-clock ticks until it reaches the alarm value, then resets the counter to zero
- Each channel shall provide enable/disable via `timercfg#K.limit` (`limit=0` disables the channel)
- Each channel shall have a dedicated interrupt line

## Architecture

- **Source clock** feeds the module.
- **Input clock** (common to all channels) is derived from the source clock by a frequency divider configured in `timerclk.div`. The divider uses the same counting semantics as a channel limit (see below).
- **Channel counter** is a 32-bit counter clocked by the input clock. The counter value is accessible by software via `timercnt#K`. It counts from zero up to `timercfg#K.limit`. When it reaches the limit, the counter restarts from zero on the next input-clock tick.
- A zero value in the limit register effectively disables the channel (the counter does not advance, interrupts are never generated).

### Clock divider operation

`timerclk.div` is a frequency divider, not a direct period register.

To form the input source clock, the module counts source-clock ticks in an internal prescaler from zero up to `div`. Each time this prescaler reaches `div`, one input-clock tick is generated and the prescaler restarts from zero.

In period terms:

```text
input_period = div * source_period        (for div > 0)
```

The prescaler semantics are analogous to a channel limit: think of `div` the same way as `timercfg#K.limit`.

If `div = 0`, the input clock is gated: prescaler and channel counters do not advance. Software may still read and write all registers at any time.

### Channel period

For an enabled channel (`limit > 0`), one full counter cycle spans `limit + 1` input-clock ticks. This supports power-of-two periods up to `2^32`.

```text
channel_period = (limit + 1) * input_period
```

## Register Interface

All timer registers are 32-bit MMIO registers. Addresses below are byte offsets from the module base.

| name         | offset | description |
| timerclk     | 0x0000 | timer module clock configuration |
| timercnt1    | 0x0004 | timer channel #1 counter value   |
| timercnt2    | 0x0008 | timer channel #2 counter value   |
| ...                                                      |
| timercnt8    | 0x0020 | timer channel #8 counter value   |
| timercfg1    | 0x0024 | timer channel #1 configuration   |
| timercfg2    | 0x0028 | timer channel #2 configuration   |
| ...                                                      |
| timercfg8    | 0x0040 | timer channel #8 configuration   |

### `timerclk` register layout

| field    | bits   | description                               |
| div      | [15:0] | read-write divider for timer source clock |
| reserved | [31:16]| read-as-zero                              |

NOTE: `div=0` gates the input clock. No interrupt pulses are generated. Values in `timercnt#K` and `timercfg#K` registers are unaffected and remain readable/writable.

### `timercnt#K` register layout

| field    | bits   | description              |
| cntval   | [31:0] | read-write counter value |
| reserved | —      | none                     |

NOTE: if software writes `cntval >= timercfg#K.limit`, the counter resets to zero on the next input-clock tick.

Each time the counter reaches `timercfg#K.limit`, the channel generates one interrupt pulse on its dedicated UCLIC line.

### `timercfg#K` register layout

| field    | bits   | description |
| limit    | [31:0] | read-write timer limit value |
| reserved | —      | none         |

NOTE: writing `limit=0` effectively disables the channel (the counter does not advance, no interrupt pulses are generated).

## RISCV-VP Integration

This section describes how the timer module is integrated into riscv-vp.

### Relation to existing components

The module is added **in addition to** the existing `BasicTimer` platform stub.

### Clock source

The module source clock is the TLM tick.

### Build-time configuration

The number of timer channels is selected at riscv-vp build time:

```c
#define TIMER_NUM_CHANNELS 8   /* default; valid range: 1...8 */
```

Channels numbered `1 ... TIMER_NUM_CHANNELS` are implemented. For channel numbers `K > TIMER_NUM_CHANNELS`, the corresponding `timercnt#K` and `timercfg#K` registers are present in the memory map but behave as read-write zeros (writes are ignored).

### Memory map

| parameter         | value                                              |
| MMIO base address | `0x02010400` (dedicated platform bus aperture)     |

The module is a standalone SimpleBus target placed after the CLINT and SyscallHandler apertures on the basic platform. It is **not** part of the CLINT register block. Register byte offsets in the register interface table above are relative to this base.

### Interrupts

Each channel connects to the platform UCLIC via a dedicated interrupt line. M/S domain routing is configured in UCLIC `SOURCECFG`.

| parameter             | value                                        |
| interrupt source base | `32` (channel #K uses source `32 + (K - 1)`) |

Interrupt sources starting at 32 avoid conflicts with major (locally injected) interrupts and with platform peripheral lines 52–57 documented in the basic platform.

When a channel counter reaches its limit, the module delivers one interrupt pulse through the platform `interrupt_gateway` interface. Software shall configure the corresponding UCLIC source for edge-triggered delivery.

### Reset and reserved bits

All registers reset to zero. Reserved register bits are read-as-zero; writes to reserved bits are ignored.
