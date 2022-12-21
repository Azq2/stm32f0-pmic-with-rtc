#pragma once

#include <cstdint>

#define COUNT_OF(array) (sizeof(array) / sizeof(array[0]))

#define DISABLE_INTERRUPTS()	__asm__ volatile ( " cpsid i " ::: "memory" )
#define ENABLE_INTERRUPTS()		__asm__ volatile ( " cpsie i " ::: "memory" )

int idec(int v, int n = 3);
int iexp(int v, int n = 3);

void uart_simple_setup(uint32_t usart, uint32_t baudrate, bool printf);

void ENTER_CRITICAL(void);
void EXIT_CRITICAL(void);
