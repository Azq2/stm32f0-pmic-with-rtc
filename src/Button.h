#pragma once

#include <delegate/delegate.hpp>
#include "Task.h"

class Button {
	public:
		enum Event {
			EVT_PRESS,
			EVT_LONGPRESS,
			EVT_RELEASE,
			EVT_LONGRELEASE
		};
		
		typedef delegate<void(void *, Event)> Callback;
	protected:
		Callback m_callback = {};
		void *m_user_data = nullptr;
		
		Task m_press_timer = {};
		Task m_release_timer = {};
		Task m_longpress_timer = {};
		
		Event m_state = EVT_RELEASE;
		
		uint32_t m_debounce = 100;
		uint32_t m_longpress_time = 5000;
		
		void onPress(void *);
		void onRelease(void *);
		void onLongPress(void *);
	public:
		void init(Callback callback, void *user_data = nullptr);
		void update(bool state);
		void handleExti(void *, bool state);
		void handleExtiInverted(void *, bool state);
		
		inline void setTimings(uint32_t debounce, uint32_t longpress_time) {
			m_debounce = debounce;
			m_longpress_time = longpress_time;
		}
		
		inline bool isPressed() {
			return m_state != EVT_RELEASE;
		}
		
		inline bool isShortPressed() {
			return m_state == EVT_PRESS;
		}
		
		inline bool isLongPressed() {
			return m_state == EVT_LONGPRESS;
		}
		
		inline bool isReleased() {
			return m_state == EVT_RELEASE;
		}
};
