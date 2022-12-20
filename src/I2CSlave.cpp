#include "I2CSlave.h"

#include "Debug.h"

#include <cstring>

#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>

bool I2CSlave::m_start = false;
void *I2CSlave::m_user_data = nullptr;

I2CSlave::ReadCallback I2CSlave::m_read_reg;
I2CSlave::WriteCallback I2CSlave::m_write_reg;

void I2CSlave::init() {
	rcc_set_i2c_clock_hsi(I2C1);
	
	i2c_reset(I2C1);
	i2c_peripheral_disable(I2C1);
	
	i2c_enable_analog_filter(I2C1);
	i2c_set_digital_filter(I2C1, 0);
	
	i2c_set_speed(I2C1, i2c_speed_sm_100k, 8);
	i2c_set_7bit_addr_mode(I2C1);
	
	i2c_set_own_7bit_slave_address(I2C1, 0x34);
	I2C_OAR1(I2C1) |= I2C_OAR1_OA1EN_ENABLE;
	
	i2c_enable_interrupt(I2C1, (
		I2C_CR1_RXIE | I2C_CR1_ADDRIE | I2C_CR1_STOPIE | I2C_CR1_ERRIE
	));
	
	i2c_enable_stretching(I2C1);
	i2c_enable_autoend(I2C1);
	I2C_CR2(I2C1) &= ~I2C_CR2_NACK;
	nvic_enable_irq(NVIC_I2C1_IRQ);
	
	i2c_peripheral_enable(I2C1);
}

void I2CSlave::handleEvent(I2CSlave::Event ev, uint8_t *byte) {
	static uint32_t rx_n = 0;
	static uint32_t tx_n = 0;
	static uint8_t tmp_rx[32] = {};
	static uint8_t tmp_tx[32] = {};
	static bool is_read = false;
	
	switch (ev) {
		case I2CSlave::EV_START_READ:
		case I2CSlave::EV_START_WRITE:
			is_read = (ev == I2CSlave::EV_START_READ);
		break;
		
		case I2CSlave::EV_STOP:
			if (rx_n == 5) {
				uint32_t value;
				memcpy(&value, &tmp_rx[1], sizeof(value));
				if (m_write_reg)
					m_write_reg(m_user_data, tmp_rx[0], value);
			}
			
			rx_n = 0;
			tx_n = 0;
			memset(&tmp_rx, 0, sizeof(tmp_rx));
			memset(&tmp_tx, 0, sizeof(tmp_tx));
		break;
		
		case I2CSlave::EV_RX:
			if (rx_n < sizeof(tmp_rx))
				tmp_rx[rx_n++] = *byte;
		break;
		
		case I2CSlave::EV_TX:
			if (is_read && tx_n == 0 && rx_n == 1 && m_read_reg) {
				uint32_t result = m_read_reg(m_user_data, tmp_rx[0]);
				memcpy(tmp_tx, &result, sizeof(result));
			}
			
			if (tx_n < sizeof(tmp_tx))
				*byte = tmp_tx[tx_n++] & 0xFF;
		break;
	}
}

void I2CSlave::irqHandler() {
	uint32_t irq_flags = I2C_ISR(I2C1);
	if ((irq_flags & I2C_ISR_STOPF)) {
		I2C_CR1(I2C1) &= ~I2C_CR1_TXIE;
		I2C_ICR(I2C1) |= I2C_ICR_STOPCF;
		
		handleEvent(EV_STOP, nullptr);
	} else if ((irq_flags & I2C_ISR_ADDR)) {
		I2C_ICR(I2C1) |= I2C_ICR_ADDRCF;
		
		if ((irq_flags & I2C_ISR_DIR_READ)) {
			I2C_ISR(I2C1) |= I2C_ISR_TXE;
			I2C_CR1(I2C1) |= I2C_CR1_TXIE;
			
			handleEvent(EV_START_READ, nullptr);
		} else {
			handleEvent(EV_START_WRITE, nullptr);
		}
	} else if ((irq_flags & (I2C_ISR_BERR | I2C_ISR_OVR))) {
		I2C_CR1(I2C1) &= ~I2C_CR1_TXIE;
		I2C_ICR(I2C1) |= I2C_ICR_BERRCF | I2C_ICR_OVRCF;
	} else if ((irq_flags & I2C_ISR_RXNE)) {
		uint8_t data = I2C_RXDR(I2C1) & 0xFF;
		handleEvent(EV_RX, &data);
	} else if ((I2C_CR1(I2C1) & I2C_CR1_TXIE) && (irq_flags & I2C_ISR_TXIS)) {
		uint8_t byte = 0xFF;
		handleEvent(EV_TX, &byte);
		I2C_TXDR(I2C1) = byte;
	}
}

void i2c1_isr() {
	I2CSlave::irqHandler();
}
