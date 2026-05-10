#ifndef UART_H
#define UART_H

/* Initialise USART2 on PA2 (TX) and PA3 (RX) at 115200 baud, 8N1.
 * Assumes the system clock is the default MSI at 4 MHz. */
void uart_init(void);

/* Transmit a single byte. Blocks until the transmit data register is empty. */
void uart_putc(char c);

/* Transmit a null-terminated string by repeatedly calling uart_putc. */
void uart_puts(const char *s);

#endif /* UART_H */
