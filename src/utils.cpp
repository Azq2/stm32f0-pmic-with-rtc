#include "utils.h"

#include <cstdio>
#include <cerrno>
#include <cmath>
#include <unistd.h>

#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/dwt.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

static uint32_t uart_for_prinf = 0;

void uart_simple_setup(uint32_t usart, uint32_t baudrate, bool use_for_printf) {
	usart_set_baudrate(usart, baudrate);
	usart_set_databits(usart, 8);
	usart_set_stopbits(usart, USART_STOPBITS_1);
	usart_set_parity(usart, USART_PARITY_NONE);
	usart_set_flow_control(usart, USART_FLOWCONTROL_NONE);
	usart_set_mode(usart, USART_MODE_TX_RX);
	usart_enable(usart);
	
	if (use_for_printf)
		uart_for_prinf = usart;
}

extern "C"
__attribute__((used))
void putchar_(char c) {
	if (uart_for_prinf)
		usart_send_blocking(uart_for_prinf, c);
}

extern "C"
__attribute__((used))
void hard_fault_handler(void) {
	printf("hard_fault_handler!\r\n");
	while (true);
}

extern "C"
__attribute__((used))
void mem_manage_handler(void) {
	printf("hard_fault_handler!\r\n");
	while (true);
}

extern "C"
__attribute__((used))
void bus_fault_handler(void) {
	printf("bus_fault_handler!\r\n");
	while (true);
}

extern "C"
__attribute__((used))
void usage_fault_handler(void) {
	printf("usage_fault_handler!\r\n");
	while (true);
}

extern "C"
__attribute__((used))
void __cxa_guard_acquire(void *g) {
	
}

extern "C"
__attribute__((used))
void __cxa_guard_release(void *g) {
	
}
