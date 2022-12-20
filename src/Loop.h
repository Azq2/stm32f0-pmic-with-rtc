#pragma once

#include <cstdint>

#include "Task.h"
#include <delegate/delegate.hpp>

class Loop {
	public:
		typedef delegate<bool(void *)> IdleCallback;
	
	protected:
		static Task *m_first;
		static Task *m_last;
		static uint32_t m_changed;
		static int64_t m_last_log;
		static volatile int64_t m_ticks;
		
		static IdleCallback m_idle_callback;
		static void *m_idle_callback_data;
	public:
		static void init();
		static void run();
		static void suspend(bool standby);
		
		static inline void setIdleCallback(IdleCallback callback, void *data = nullptr) {
			m_idle_callback = callback;
			m_idle_callback_data = data;
		}
		
		static int64_t ms() {
			return m_ticks;
		}
		
		static uint32_t log() {
			int64_t now = ms();
			uint32_t result = (m_last_log ? now - m_last_log : 0);
			m_last_log = now;
			return result;
		}
		
		static inline Task *first() {
			return m_first;
		}
		
		static inline void first(Task *task) {
			m_first = task;
		}
		
		static inline Task *last() {
			return m_last;
		}
		
		static inline void last(Task *task) {
			m_last = task;
		}
		
		static inline void tick() {
			m_ticks++;
		}
		
		static inline void onChange() {
			m_changed++;
		}
		
		static void idleFor(uint32_t idle_time);
};
