#ifndef RISCV_ISA_EXTERNAL_TIMER_H
#define RISCV_ISA_EXTERNAL_TIMER_H

#include <array>
#include <cstdint>

#include "core/common/irq_if.h"
#include "util/tlm_map.h"

#include <tlm_core/tlm_2/tlm_quantum/tlm_global_quantum.h>
#include <tlm_utils/simple_target_socket.h>
#include <systemc>

#ifndef TIMER_NUM_CHANNELS
#define TIMER_NUM_CHANNELS 8
#endif

static_assert(TIMER_NUM_CHANNELS >= 1 && TIMER_NUM_CHANNELS <= 8);

struct ExternalTimer : public sc_core::sc_module {
	static constexpr unsigned MAX_CHANNELS = 8;
	static constexpr uint32_t IRQ_BASE = 32;

	static constexpr uint64_t TIMERCLK_ADDR = 0x0000;
	static constexpr uint64_t TIMERCNT_ADDR = 0x0004;
	static constexpr uint64_t TIMERCNT_STRIDE = 0x0004;
	static constexpr uint64_t TIMERCNT_LAST_ADDR = 0x0020;
	static constexpr uint64_t TIMERCFG_ADDR = 0x0024;
	static constexpr uint64_t TIMERCFG_STRIDE = 0x0004;
	static constexpr uint64_t TIMERCFG_LAST_ADDR = 0x0040;
	static constexpr uint64_t REGISTER_SPACE_SIZE = 0x0044;

	tlm_utils::simple_target_socket<ExternalTimer> tsock;

	interrupt_gateway *plic = nullptr;

	sc_core::sc_event tick_event;
	sc_core::sc_time last_update_time = sc_core::SC_ZERO_TIME;

	uint32_t timerclk = 0;
	std::array<uint32_t, MAX_CHANNELS> timercnt{};
	std::array<uint32_t, MAX_CHANNELS> timercfg{};

	uint32_t prescaler = 0;
	std::array<bool, MAX_CHANNELS> pending_reset{};

	vp::map::LocalRouter router;

	SC_HAS_PROCESS(ExternalTimer);

	ExternalTimer(sc_core::sc_module_name) {
		tsock.register_b_transport(this, &ExternalTimer::transport);

		std::vector<vp::map::reg_mapping_t> regs;
		regs.push_back({TIMERCLK_ADDR, &timerclk, vp::map::read_write, 0x0000FFFF});

		for (unsigned k = 0; k < MAX_CHANNELS; ++k) {
			regs.push_back({TIMERCNT_ADDR + k * TIMERCNT_STRIDE, &timercnt[k]});
			regs.push_back({TIMERCFG_ADDR + k * TIMERCFG_STRIDE, &timercfg[k]});
		}

		router.add_register_bank(regs).register_handler(this, &ExternalTimer::register_access_callback);

		SC_THREAD(run);
		schedule_next();
	}

	uint16_t div() const {
		return static_cast<uint16_t>(timerclk & 0xFFFF);
	}

	bool channel_implemented(unsigned k) const {
		return k < static_cast<unsigned>(TIMER_NUM_CHANNELS);
	}

	void register_access_callback(const vp::map::register_access_t &r) {
		const auto addr = r.addr;

		if (addr >= TIMERCNT_ADDR && addr <= TIMERCNT_LAST_ADDR) {
			const unsigned k = static_cast<unsigned>((addr - TIMERCNT_ADDR) / TIMERCNT_STRIDE);
			if (!channel_implemented(k)) {
				if (r.write)
					return;
				const uint32_t saved = *r.vptr;
				*r.vptr = 0;
				r.fn();
				*r.vptr = saved;
				return;
			}

			if (r.read)
				advance_to(sc_core::sc_time_stamp() + r.delay);

			r.fn();

			if (r.write)
				on_timercnt_write(k);
			return;
		}

		if (addr >= TIMERCFG_ADDR && addr <= TIMERCFG_LAST_ADDR) {
			const unsigned k = static_cast<unsigned>((addr - TIMERCFG_ADDR) / TIMERCFG_STRIDE);
			if (!channel_implemented(k)) {
				if (r.write)
					return;
				const uint32_t saved = *r.vptr;
				*r.vptr = 0;
				r.fn();
				*r.vptr = saved;
				return;
			}

			r.fn();

			if (r.write)
				on_timercfg_write(k);
			return;
		}

		if (addr == TIMERCLK_ADDR) {
			r.fn();
			if (r.write)
				on_timerclk_write();
			return;
		}

		r.fn();
	}

	void on_timerclk_write() {
		advance_to(sc_core::sc_time_stamp());
		schedule_next();
	}

	void on_timercfg_write(unsigned k) {
		advance_to(sc_core::sc_time_stamp());
		if (timercfg[k] == 0)
			pending_reset[k] = false;
		schedule_next();
	}

	void on_timercnt_write(unsigned k) {
		advance_to(sc_core::sc_time_stamp());
		if (timercfg[k] > 0 && timercnt[k] >= timercfg[k])
			pending_reset[k] = true;
		schedule_next();
	}

	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
		router.transport(trans, delay);
	}

	void run() {
		while (true) {
			sc_core::wait(tick_event);
			advance_to(sc_core::sc_time_stamp());
			schedule_next();
		}
	}

	sc_core::sc_time source_period() const {
		return tlm::tlm_global_quantum::instance().get();
	}

	void advance_to(sc_core::sc_time now) {
		const uint16_t divider = div();
		if (divider == 0) {
			last_update_time = now;
			return;
		}

		const sc_core::sc_time period = source_period();
		if (period == sc_core::SC_ZERO_TIME) {
			last_update_time = now;
			return;
		}

		if (last_update_time == sc_core::SC_ZERO_TIME)
			last_update_time = now;

		if (now <= last_update_time)
			return;

		const uint64_t elapsed_source_ticks =
		    static_cast<uint64_t>((now - last_update_time) / period);
		if (elapsed_source_ticks == 0)
			return;

		last_update_time += sc_core::sc_time(static_cast<double>(elapsed_source_ticks) * period);

		const uint64_t total = static_cast<uint64_t>(prescaler) + elapsed_source_ticks;
		const uint64_t input_ticks = total / divider;
		prescaler = static_cast<uint32_t>(total % divider);

		process_input_ticks(input_ticks);
	}

	void process_input_ticks(uint64_t n) {
		if (n == 0)
			return;

		for (unsigned k = 0; k < static_cast<unsigned>(TIMER_NUM_CHANNELS); ++k)
			channel_advance_batch(k, n);
	}

	void channel_advance_batch(unsigned k, uint64_t n) {
		const uint32_t limit = timercfg[k];
		if (limit == 0 || n == 0)
			return;

		const uint32_t period = limit + 1;

		if (pending_reset[k]) {
			timercnt[k] = 0;
			pending_reset[k] = false;
			--n;
		}

		if (n == 0)
			return;

		const uint64_t pulses = count_limit_pulses(timercnt[k], limit, n);
		timercnt[k] = static_cast<uint32_t>((static_cast<uint64_t>(timercnt[k]) + n) % period);
		trigger_irq_pulses(k, pulses);
	}

	static uint64_t count_limit_pulses(uint32_t counter, uint32_t limit, uint64_t input_ticks) {
		const uint64_t period = static_cast<uint64_t>(limit) + 1;
		uint64_t delta = (static_cast<uint64_t>(limit) - counter) % period;

		if (delta == 0)
			delta = period;

		if (input_ticks < delta)
			return 0;

		return 1 + (input_ticks - delta) / period;
	}

	void trigger_irq_pulses(unsigned k, uint64_t pulses) {
		if (plic == nullptr || pulses == 0)
			return;

		const uint32_t irq_id = IRQ_BASE + k;
		for (uint64_t i = 0; i < pulses; ++i)
			plic->gateway_trigger_interrupt(irq_id);
	}

	void schedule_next() {
		tick_event.cancel();

		const uint16_t divider = div();
		if (divider == 0)
			return;

		const sc_core::sc_time period = source_period();
		if (period == sc_core::SC_ZERO_TIME)
			return;

		if (last_update_time == sc_core::SC_ZERO_TIME)
			last_update_time = sc_core::sc_time_stamp();

		const sc_core::sc_time next_source_tick = last_update_time + period;
		const sc_core::sc_time now = sc_core::sc_time_stamp();
		const sc_core::sc_time delay = next_source_tick > now ? next_source_tick - now : sc_core::SC_ZERO_TIME;
		tick_event.notify(delay);
	}
};

#endif  // RISCV_ISA_EXTERNAL_TIMER_H
