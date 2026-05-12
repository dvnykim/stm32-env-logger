#include "stm32l476xx.h"
#include "uart.h"
#include "i2c.h"
#include "sht31.h"

static void crude_delay(uint32_t loops) {
    for (volatile uint32_t i = 0; i < loops; i++) { }
}

int main(void) {
    /* --- LED setup --- */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    GPIOA->MODER &= ~(0b11U << (5 * 2));
    GPIOA->MODER |=  (0b01U << (5 * 2));

    /* --- Peripherals --- */
    uart_init();
    i2c_init();

    uart_puts("\r\n--- STM32L476 + SHT31 boot ---\r\n");

    while (1) {
        GPIOA->ODR ^= (1U << 5);                /* heartbeat LED */

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

        crude_delay(800000);                    /* ~1 sample/sec at 4 MHz */
    }
}
