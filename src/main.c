#include "stm32l476xx.h"
#include "uart.h"

int main(void) {
    /* --- LED setup (from Phase 1) --- */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER &= ~(0b11U << (5 * 2));
    GPIOA->MODER |=  (0b01U << (5 * 2));

    /* --- UART setup (Phase 2) --- */
    uart_init();
    uart_puts("\r\nSTM32L476 boot OK. UART alive.\r\n");

    uint32_t tick = 0;
    while (1) {
        GPIOA->ODR ^= (1U << 5);
        uart_puts("tick\r\n");
        tick++;
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}
