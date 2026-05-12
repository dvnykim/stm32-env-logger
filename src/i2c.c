#include "stm32l476xx.h"
#include "i2c.h"

/* Timing register for I2C1 at 100 kHz standard mode, kernel clock = 4 MHz.
 *
 *   PRESC = 0      → t_PRESC = 1 / 4MHz = 250 ns
 *   SCLDEL = 3     → setup time = 4 × 250 ns = 1000 ns (Sm requires ≥250)
 *   SDADEL = 1     → hold time  = 1 × 250 ns =  250 ns
 *   SCLH = 0x0F    → high time  = 16 × 250 ns = 4000 ns
 *   SCLL = 0x17    → low time   = 24 × 250 ns = 6000 ns
 *
 * Period ≈ 4000 + 6000 = 10000 ns → 100 kHz exactly. */
#define I2C_TIMINGR_100KHZ_4MHZ  0x00310F17U

/* A coarse spin-loop limit. At 4 MHz core clock and ~5 instructions per
 * iteration, 100,000 iterations is roughly 100 ms — long enough that a
 * working transaction never hits it, short enough that we don't hang
 * for minutes when the sensor is unplugged. */
#define I2C_TIMEOUT  100000U

void i2c_init(void) {
    /* ----- 1. Peripheral clocks --------------------------------------- */
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOBEN;       /* GPIOB on AHB2 */
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;       /* I2C1 on APB1 */

    /* ----- 2. GPIO PB8 (SCL) and PB9 (SDA) ---------------------------- */
    /* MODER → alternate function (10) */
    GPIOB->MODER &= ~(0b11U << (8 * 2));
    GPIOB->MODER |=  (0b10U << (8 * 2));        /* PB8 = AF */
    GPIOB->MODER &= ~(0b11U << (9 * 2));
    GPIOB->MODER |=  (0b10U << (9 * 2));        /* PB9 = AF */

    /* OTYPER → open-drain (1). MANDATORY for I2C — pins must never
     * actively drive high, only let pull-ups raise them. */
    GPIOB->OTYPER |= (1U << 8) | (1U << 9);

    /* PUPDR → pull-up (01). Belt and braces — enables the internal
     * weak pull-ups so the bus still works if your breakout has none.
     * If you DO have external pull-ups, this is harmless. */
    GPIOB->PUPDR &= ~(0b11U << (8 * 2));
    GPIOB->PUPDR |=  (0b01U << (8 * 2));
    GPIOB->PUPDR &= ~(0b11U << (9 * 2));
    GPIOB->PUPDR |=  (0b01U << (9 * 2));

    /* OSPEEDR → medium speed is plenty for 100 kHz. (Reset value works
     * but being explicit doesn't hurt.) */
    GPIOB->OSPEEDR &= ~(0b11U << (8 * 2));
    GPIOB->OSPEEDR |=  (0b01U << (8 * 2));
    GPIOB->OSPEEDR &= ~(0b11U << (9 * 2));
    GPIOB->OSPEEDR |=  (0b01U << (9 * 2));

    /* AFR — pick the right alternate function. From the L476 datasheet
     * pin alternate function table: I2C1_SCL/SDA on PB8/PB9 = AF4.
     * Pins 8-15 are configured in AFR[1] (a.k.a. AFRH). Pin 8 is bits
     * [3:0] of AFRH, pin 9 is bits [7:4]. */
    GPIOB->AFR[1] &= ~(0xFU << ((8 - 8) * 4));
    GPIOB->AFR[1] |=  (4U   << ((8 - 8) * 4));   /* PB8 → AF4 */
    GPIOB->AFR[1] &= ~(0xFU << ((9 - 8) * 4));
    GPIOB->AFR[1] |=  (4U   << ((9 - 8) * 4));   /* PB9 → AF4 */

    /* ----- 3. Configure I2C1 ------------------------------------------ */
    /* Disable peripheral while we write TIMINGR (required by hardware). */
    I2C1->CR1 &= ~I2C_CR1_PE;

    I2C1->TIMINGR = I2C_TIMINGR_100KHZ_4MHZ;

    /* Default CR1: analog filter on (ANFOFF=0), digital filter off (DNF=0),
     * no interrupts, no DMA. Just enable the peripheral. */
    I2C1->CR1 = I2C_CR1_PE;
}

/* Helper: wait for a specific flag in ISR to become set.
 * Returns 0 on success, -1 on timeout. Also bails out on NACK. */
static int wait_flag(uint32_t flag) {
    uint32_t timeout = I2C_TIMEOUT;
    while (timeout--) {
        uint32_t isr = I2C1->ISR;
        if (isr & flag)         return 0;
        if (isr & I2C_ISR_NACKF) return -1;   /* slave didn't ACK */
    }
    return -1;                                /* hard timeout */
}

int i2c_write(uint8_t addr, const uint8_t *data, size_t len) {
    /* Build the CR2 word that describes the whole transaction:
     *   - SADD bits [7:1] hold the 7-bit address (shifted left by 1).
     *   - RD_WRN = 0 for write.
     *   - NBYTES bits [23:16] = number of bytes to send.
     *   - AUTOEND = 1 → hardware emits STOP after NBYTES. */
    I2C1->CR2 = ((uint32_t)addr << 1)
              | ((uint32_t)len  << 16)
              | I2C_CR2_AUTOEND;

    /* Set START. Hardware drives START + address; if slave NACKs the
     * address, NACKF goes up and our wait_flag will return -1. */
    I2C1->CR2 |= I2C_CR2_START;

    /* Send each byte: wait for TXIS (TX register empty / "feed me"),
     * then write a byte to TXDR. */
    for (size_t i = 0; i < len; i++) {
        if (wait_flag(I2C_ISR_TXIS) < 0) return -1;
        I2C1->TXDR = data[i];
    }

    /* Wait for the STOP that AUTOEND will generate, then clear the flag. */
    if (wait_flag(I2C_ISR_STOPF) < 0) return -1;
    I2C1->ICR = I2C_ICR_STOPCF;
    return 0;
}

int i2c_read(uint8_t addr, uint8_t *data, size_t len) {
    I2C1->CR2 = ((uint32_t)addr << 1)
              | ((uint32_t)len  << 16)
              | I2C_CR2_RD_WRN
              | I2C_CR2_AUTOEND;
    I2C1->CR2 |= I2C_CR2_START;

    for (size_t i = 0; i < len; i++) {
        if (wait_flag(I2C_ISR_RXNE) < 0) return -1;
        data[i] = (uint8_t)I2C1->RXDR;
    }

    if (wait_flag(I2C_ISR_STOPF) < 0) return -1;
    I2C1->ICR = I2C_ICR_STOPCF;
    return 0;
}
