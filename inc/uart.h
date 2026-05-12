#ifndef UART_H
#define UART_H

#include <stdint.h>

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);

/* Print a signed decimal integer (no padding, base 10). */
void uart_print_int(int32_t value);

/* Print a value scaled by 100 as "WW.FF". E.g. 2347 prints as "23.47". */
void uart_print_x100(int32_t value_x100);

#endif

