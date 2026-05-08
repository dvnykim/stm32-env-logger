#include "stm32l476xx.h"

int main(void) {
    /* 1. Enable the clock for GPIO Port A.
     *    GPIOA sits on the AHB2 bus. Bit 0 of RCC->AHB2ENR is the enable bit. */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    /* 2. Configure PA5 as a general-purpose output.
     *    MODER stores 2 bits per pin. Pin 5's bits are at positions [11:10].
     *    Clear them first, then write 01 (output mode). */
    GPIOA->MODER &= ~(0b11U << (5 * 2));
    GPIOA->MODER |=  (0b01U << (5 * 2));

    /* 3. Blink loop: toggle PA5, then waste time. */
    while (1) {
        GPIOA->ODR ^= (1U << 5);
        for (volatile uint32_t i = 0; i < 300000; i++);
    }
}
