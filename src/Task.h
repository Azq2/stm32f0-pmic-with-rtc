#pragma once

#include <delegate/delegate.hpp>
#include <cstdint>

class Task {
	public:
		typedef delegate<void(void *)> Callback;
	
	protected:
		Callback m_callback;
		void *m_user_data = nullptr;
		Task *m_next = nullptr;
		Task *m_prev = nullptr;
		bool m_loop = false;
		bool m_enabled = false;
		int64_t m_next_run = 0;
		uint32_t m_interval = 0;
		
		friend class Loop;
	public:
		Task() {
			
		}
		
		void exec();
		
		inline bool enabled() {
			return m_enabled;
		}
		
		inline int64_t nextRun() {
			return m_next_run;
		}
		
		inline Task *prev() {
			return m_prev;
		}
		
		inline Task *next() {
			return m_next;
		}
		
		inline void init(Callback callback, void *user_data = nullptr) {
			m_callback = callback;
			m_user_data = user_data;
		}
		
		void run(uint32_t ms, bool loop);
		
		inline void setTimeout(uint32_t ms) {
			run(ms, false);
		}
		
		inline void setInterval(uint32_t ms) {
			run(ms, true);
		}
		
		void cancel();
};
