#include "Exti.h"

#include <libopencm3/stm32/gpio.h>

#include <cstdio>

Exti::Channel Exti::m_channels[EXTI_COUNT];

int Exti::enable(uint32_t bank, uint32_t pin) {
	uint8_t id = Gpio::pin2id(pin);
	uint32_t exti = id2exti(id);
	
	if (!m_channels[id].bank || m_channels[id].bank != bank)
		return ERR_NOT_EXISTS;
	exti_enable_request(exti);
	return ERR_SUCCESS;
}

int Exti::disable(uint32_t bank, uint32_t pin) {
	int id = Gpio::pin2id(pin);
	uint32_t exti = id2exti(id);
	
	if (!m_channels[id].bank || m_channels[id].bank != bank)
		return ERR_SUCCESS;
	exti_disable_request(exti);
	return ERR_SUCCESS;
}

int Exti::set(uint32_t bank, uint32_t pin, Trigger trigger, Callback callback, void *user_data) {
	uint8_t id = Gpio::pin2id(pin);
	uint32_t exti = id2exti(id);
	
	if (m_channels[id].bank)
		return ERR_EXISTS;
	
	m_channels[id].bank = bank;
	m_channels[id].callback = callback;
	m_channels[id].user_data = user_data;
	
	// Enable IRQ for EXTI
	int irq_n = getIrq(id);
	nvic_enable_irq(irq_n);
	
	// Enable EXTI and connect to specified GPIO bank
	exti_select_source(exti, bank);
	exti_set_trigger(exti, (exti_trigger_type) trigger);
	return enable(bank, pin);
}

int Exti::remove(uint32_t bank, uint32_t pin) {
	int id = Gpio::pin2id(pin);
	uint32_t exti = id2exti(id);
	
	if (!m_channels[id].bank || m_channels[id].bank != bank)
		return ERR_SUCCESS;
	
	m_channels[id].bank = 0;
	m_channels[id].callback = nullptr;
	m_channels[id].user_data = nullptr;
	
	// Disconnect GPIO bank from EXTI
	exti_select_source(exti, 0);
	
	// Disable IRQ, if it not used by another EXTI
	int irq_n = getIrq(id);
	if (!isSharedIrqUsed(irq_n))
		nvic_disable_irq(irq_n);
	
	return disable(bank, pin);
}

void Exti::handleIrq(uint8_t id) {
	uint32_t exti = id2exti(id);
	Channel &channel = m_channels[id];
	
	bool state = false;
	if ((EXTI_RTSR & exti) && (EXTI_FTSR & exti)) {
		uint16_t pin = Gpio::id2pin(id);
		state = gpio_get(channel.bank, pin) != 0;
	} else if ((EXTI_RTSR & exti)) {
		state = true;
	}
	
	exti_reset_request(exti);
	channel.callback(channel.user_data, state);
}

void Exti::handleIrqRange(uint8_t from, uint8_t to) {
	for (uint8_t i = from; i <= to; i++) {
		if (exti_get_flag_status(id2exti(i)))
			Exti::handleIrq(i);
	}
}

int Exti::getIrq(uint8_t id) {
	switch (id) {
		case 0:		return NVIC_EXTI0_1_IRQ;
		case 1:		return NVIC_EXTI0_1_IRQ;
		case 2:		return NVIC_EXTI2_3_IRQ;
		case 3:		return NVIC_EXTI2_3_IRQ;
		case 4: case 5: case 6: case 7: case 8: case 9:
		case 10: case 11: case 12: case 13: case 14: case 15:
			return NVIC_EXTI4_15_IRQ;
	}
	return -1;
}

bool Exti::isSharedIrqUsed(int irq_n) {
	switch (irq_n) {
		case NVIC_EXTI0_1_IRQ:
			for (int i = 0; i <= 1; i++) {
				if (m_channels[i].bank)
					return true;
			}
		break;
		
		case NVIC_EXTI2_3_IRQ:
			for (int i = 2; i <= 3; i++) {
				if (m_channels[i].bank)
					return true;
			}
		break;
		
		case NVIC_EXTI4_15_IRQ:
			for (int i = 4; i <= 15; i++) {
				if (m_channels[i].bank)
					return true;
			}
		break;
	}
	return false;
}

extern "C" void exti0_1_isr(void) {
	Exti::handleIrqRange(0, 1);
}

extern "C" void exti2_3_isr(void) {
	Exti::handleIrqRange(2, 3);
}

extern "C" void exti4_15_isr(void) {
	Exti::handleIrqRange(4, 15);
}
