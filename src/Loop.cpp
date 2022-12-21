#include "Loop.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/systick.h>
#include <algorithm>
#include <cstdio>

Task *Loop::m_first = nullptr;
Task *Loop::m_last = nullptr;
uint32_t Loop::m_changed = 0;
int64_t Loop::m_last_log = 0;
volatile int64_t Loop::m_ticks = 0;

uint32_t m_max_idle_time = 0;
uint32_t m_counts_per_tick = 0;

Loop::IdleCallback Loop::m_idle_callback;
void *Loop::m_idle_callback_data = nullptr;

void Loop::init() {
	m_counts_per_tick = rcc_ahb_frequency / 8 / 1000;
	m_max_idle_time = 0xFFFFFF / m_counts_per_tick;
	
	// printf("m_max_idle_time=%ld / m_counts_per_tick=%ld\r\n", m_max_idle_time, m_counts_per_tick);
	
	systick_set_clocksource(STK_CSR_CLKSOURCE_EXT);
	STK_CVR = 0;
	systick_set_reload(m_counts_per_tick);
	systick_counter_enable();
	systick_interrupt_enable();
}

void Loop::run() {
	while (true) {
		Task *task = m_first;
		int64_t m_next_run = ms() + m_max_idle_time;
		uint32_t changed_before = m_changed;
		int processed = 0;
		
		while (task) {
			Task *next = task->next();
			if (task->enabled()) {
				if (ms() >= task->nextRun())
					task->exec();
				
				if (task->enabled())
					m_next_run = std::min(m_next_run, task->nextRun());
				
				processed++;
			}
			task = next;
		}
		
		if (changed_before == m_changed) {
			if (!processed) {
				if (m_idle_callback && m_idle_callback(m_idle_callback_data))
					continue;
			}
			
			int64_t time_for_idle = m_next_run - ms();
			if (time_for_idle == 1) {
				__asm__ volatile("wfi");
			} else if (time_for_idle > 1) {
				idleFor(time_for_idle);
			}
		}
	}
}

void Loop::suspend(bool standby) {
	pwr_clear_standby_flag();
	pwr_clear_wakeup_flag();
	pwr_enable_wakeup_pin();
	
	if (standby) {
		pwr_disable_power_voltage_detect();
		pwr_set_standby_mode();
	} else {
		pwr_voltage_regulator_on_in_stop();
		pwr_set_stop_mode();
	}
	
	SCB_SCR |= SCB_SCR_SLEEPDEEP;
	
	__asm__ volatile("wfi");
	
	SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
}

void Loop::idleFor(uint32_t idle_time) {
	idle_time = std::min(idle_time, m_max_idle_time);
	
	// Disable systick
	systick_counter_disable();
	
	// Disable irq
	__asm__ volatile("cpsid i" ::: "memory");
	__asm__ volatile("dsb");
	__asm__ volatile("isb");
	
	// Setup systick for waiting idle_time ms
	uint32_t reload_val = systick_get_value() + (m_counts_per_tick * (idle_time - 1));
	systick_set_reload(reload_val);
	STK_CVR = 0;
	systick_counter_enable();
	
	// Do sleep
	__asm__ volatile("dsb" ::: "memory");
	__asm__ volatile("wfi");
	__asm__ volatile("isb");
	
	#if 0
	// Allow process irq fired after sleep
	__asm__ volatile("cpsie i" ::: "memory");
	__asm__ volatile("dsb");
	__asm__ volatile("isb");
	__asm__ volatile("cpsid i" ::: "memory");
	__asm__ volatile("dsb");
	__asm__ volatile("isb");
	#endif
	
	// Recalc internal ticks counter
	bool counted_to_zero = (STK_CSR & STK_CSR_COUNTFLAG) != 0;
	systick_counter_disable();
	
	if (counted_to_zero) {
		m_ticks += idle_time - 1;
	} else {
		m_ticks += ((idle_time * m_counts_per_tick) - systick_get_value()) / m_counts_per_tick;
	}
	
	// Restart systick
	STK_CVR = 0;
	systick_set_reload(m_counts_per_tick);
	systick_counter_enable();
	
	// Enable irq
	__asm__ volatile("cpsie i" ::: "memory");
}

extern "C" void sys_tick_handler(void) {
	Loop::tick();
}
