#pragma once


#include <cstdint>
#include <delegate/delegate.hpp>

class I2CSlave {
	public:
		enum Event {
			EV_START_READ,
			EV_START_WRITE,
			EV_RX,
			EV_TX,
			EV_STOP
		};
		
		typedef delegate<uint32_t(void *, uint8_t)> ReadCallback;
		typedef delegate<void(void *, uint8_t, uint32_t)> WriteCallback;
	
	protected:
		static bool m_start;
		static ReadCallback m_read_reg;
		static WriteCallback m_write_reg;
		static void *m_user_data;
	
	public:
		static void init();
		static void irqHandler();
		static void handleEvent(Event ev, uint8_t *byte);
		
		static inline void setCallback(ReadCallback read_reg, WriteCallback write_reg, void *user_data = nullptr) {
			m_read_reg = read_reg;
			m_write_reg = write_reg;
			m_user_data = user_data;
		}
};
