#pragma once

#include <cstdint>

#define COUNT_OF(array) (sizeof(array) / sizeof(array[0]))

inline int idec(int v, int n = 3) {
	int div = 0;
	for (int i = 0; i < n; i++)
		div = (div ? div * 10 : 10);
	return div ? v / div : 0;
}

inline int iexp(int v, int n = 3) {
	int div = 0;
	for (int i = 0; i < n; i++)
		div = (div ? div * 10 : 10);
	return (v - (idec(v, n) * div)) * 10 / div;
}

void uart_simple_setup(uint32_t usart, uint32_t baudrate, bool printf);

void delay_init(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t us);
