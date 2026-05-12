#include "stm32l476xx.h"
#include "uart.h"
#include "i2c.h"
#include "sht31.h"
#include "timer.h"

int main(void) {
    /* --- LED setup --- */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER &= ~(0b11U << (5 * 2));
    GPIOA->MODER |=  (0b01U << (5 * 2));

    /* --- Peripherals --- */
    uart_init();
    i2c_init();
    timer_init_2s();

    uart_puts("\r\n--- STM32L476 + SHT31 (timer-driven) ---\r\n");

    while (1) {
        if (sample_ready) {
            sample_ready = 0;            /* consume the flag */
            GPIOA->ODR ^= (1U << 5);     /* heartbeat */

            int32_t t_x100, h_x100;
            int rc = sht31_read(&t_x100, &h_x100);

            if (rc == 0) {
                uart_puts("Temp: ");
                uart_print_x100(t_x100);
                uart_puts(" C, Humidity: ");
                uart_print_x100(h_x100);
                uart_puts(" %RH\r\n");
            } else {
                uart_puts("SHT31 error: ");
                uart_print_int(rc);
                uart_puts("\r\n");
            }
        }
        /* Loop runs as fast as the CPU can spin. In a real product you'd
         * add `__WFI();` here to put the CPU to sleep until the next
         * interrupt — saves power. For now, the busy spin is fine. */
    }
}
