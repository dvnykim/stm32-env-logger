#include "stm32l476xx.h"
#include "uart.h"

/* Peripheral clock for USART2 in this project = MSI = 4 MHz.
 * BRR for 115200 baud at 16x oversampling: 4_000_000 / 115_200 ≈ 35. */
#define USART2_BRR_VALUE 35U

void uart_init(void) {
    /* ----- 1. Enable peripheral clocks -------------------------------- */
    /* GPIOA is on AHB2. The Phase 1 LED code already enabled it, but
     * doing it again is harmless (it's an OR-set of a bit that's already 1). */
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;

    /* USART2 sits on the APB1 peripheral bus. Bit 17 of APB1ENR1 is the
     * enable bit. The CMSIS macro saves us looking it up. */
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    /* ----- 2. Configure PA2 and PA3 as Alternate Function ------------- */
    /* MODER holds 2 bits per pin: 00=input, 01=output, 10=AF, 11=analog.
     * For each pin, clear-then-set so we don't depend on the reset value. */
    GPIOA->MODER &= ~(0b11U << (2 * 2));      /* clear PA2 mode bits */
    GPIOA->MODER |=  (0b10U << (2 * 2));      /* PA2 = AF mode (10) */
    GPIOA->MODER &= ~(0b11U << (3 * 2));      /* clear PA3 mode bits */
    GPIOA->MODER |=  (0b10U << (3 * 2));      /* PA3 = AF mode (10) */

    /* The "Alternate Function" register selects WHICH peripheral takes the
     * pin. There are 16 alternate functions per pin (AF0..AF15). The
     * datasheet alternate-function mapping table tells us:
     *     PA2 → USART2_TX is AF7
     *     PA3 → USART2_RX is AF7
     * AFR[0] (a.k.a. AFRL) covers pins 0..7, 4 bits per pin.
     * Pin 2 is bits [11:8], pin 3 is bits [15:12]. */
    GPIOA->AFR[0] &= ~(0xFU << (2 * 4));
    GPIOA->AFR[0] |=  (7U   << (2 * 4));      /* PA2 → AF7 */
    GPIOA->AFR[0] &= ~(0xFU << (3 * 4));
    GPIOA->AFR[0] |=  (7U   << (3 * 4));      /* PA3 → AF7 */

    /* ----- 3. Configure USART2 ---------------------------------------- */
    /* Order matters slightly: set everything, then enable the USART last. */

    /* Disable USART before reconfiguring (safe even if it was already off). */
    USART2->CR1 = 0;

    /* Baud rate. Default frame format (8 data bits, 1 stop, no parity = 8N1)
     * is what we get with CR1 = 0 and CR2 = 0. */
    USART2->BRR = USART2_BRR_VALUE;

    /* Enable transmitter (TE = bit 3) and the USART itself (UE = bit 0).
     * We're not using RX yet, but enabling RE later is one extra bit set
     * away when we need it. */
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

void uart_putc(char c) {
    /* The Transmit Data Register Empty flag (TXE, bit 7 of ISR) goes high
     * when the previous byte has been moved into the shift register and a
     * new one can be loaded. Spin until it's empty. */
    while ((USART2->ISR & USART_ISR_TXE) == 0) {
        /* nothing — just wait */
    }
    USART2->TDR = (uint8_t)c;
}

void uart_puts(const char *s) {
    while (*s) {
        uart_putc(*s++);
    }
}

void uart_print_int(int32_t value) {
    /* Worst case: "-2147483648" = 11 chars + null. */
    char buf[12];
    int  i = 0;
    int  negative = 0;
    uint32_t v;

    if (value < 0) {
        negative = 1;
        v = (uint32_t)(-value);
    } else {
        v = (uint32_t)value;
    }

    /* Build digits in reverse. */
    if (v == 0) {
        buf[i++] = '0';
    } else {
        while (v > 0) {
            buf[i++] = (char)('0' + (v % 10));
            v /= 10;
        }
    }
    if (negative) buf[i++] = '-';

    /* Reverse and send. */
    while (i--) uart_putc(buf[i]);
}

void uart_print_x100(int32_t value_x100) {
    if (value_x100 < 0) {
        uart_putc('-');
        value_x100 = -value_x100;
    }
    int32_t whole = value_x100 / 100;
    int32_t frac  = value_x100 % 100;
    uart_print_int(whole);
    uart_putc('.');
    if (frac < 10) uart_putc('0');   /* leading zero, e.g. ".07" not ".7" */
    uart_print_int(frac);
}
