#pragma once

#include "Gpio.h"

#include <cstdio>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include <delegate/delegate.hpp>

class Exti {
	public:
		enum Trigger: int {
			RISING		= EXTI_TRIGGER_RISING,
			FALLING		= EXTI_TRIGGER_FALLING,
			BOTH		= EXTI_TRIGGER_BOTH
		};
		
		typedef delegate<void(void *, bool)> Callback;
	protected:
		
		static constexpr int EXTI_COUNT = 16;
		
		static constexpr uint32_t id2exti(int id) {
			return 1 << id;
		}
		
		struct Channel {
			Callback callback;
			void *user_data = nullptr;
			uint32_t bank = 0;
		};
		
		static Channel m_channels[EXTI_COUNT];
		
		
		static int getIrq(uint8_t id);
		static bool isSharedIrqUsed(int irq_n);
	public:
		enum Errors {
			ERR_SUCCESS		= 0,
			ERR_EXISTS		= -1,
			ERR_NOT_EXISTS	= -2,
		};
		
		static void handleIrq(uint8_t id);
		static void handleIrqRange(uint8_t from, uint8_t to);
		static int set(uint32_t bank, uint32_t pin, Trigger trigger, Callback callback, void *user_data = nullptr);
		static int remove(uint32_t bank, uint32_t pin);
		static int enable(uint32_t bank, uint32_t pin);
		static int disable(uint32_t bank, uint32_t pin);
};
