#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stddef.h>

/* Initialise I2C1 on PB8 (SCL) and PB9 (SDA) at ~100 kHz standard mode.
 * Assumes the I2C kernel clock is the default PCLK1 = MSI = 4 MHz. */
void i2c_init(void);

/* Write `len` bytes to the 7-bit slave address `addr`.
 * Returns 0 on success, negative on error (NACK / timeout). */
int i2c_write(uint8_t addr, const uint8_t *data, size_t len);

/* Read `len` bytes from the 7-bit slave address `addr`.
 * Returns 0 on success, negative on error. */
int i2c_read(uint8_t addr, uint8_t *data, size_t len);

#endif /* I2C_H */
