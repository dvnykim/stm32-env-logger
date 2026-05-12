#include "stm32l476xx.h"
#include "i2c.h"
#include "sht31.h"

#define SHT31_ADDR     0x44
#define MEAS_HIGH_REP  0x2400   /* single-shot, high repeatability, no clock stretch */

/* CRC-8, polynomial 0x31 (x^8 + x^5 + x^4 + 1), init 0xFF. From SHT3x
 * datasheet section 4.12. Used to verify each 2-byte measurement word. */
static uint8_t sht31_crc8(const uint8_t *data, int len) {
    uint8_t crc = 0xFF;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31)
                               : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

/* Crude millisecond-ish delay using a calibrated busy loop. We're at
 * 4 MHz core; roughly 4000 cycles per ms. The compiler will keep the
 * loop because i is volatile. Good enough for "wait 20 ms"-style needs. */
static void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        for (volatile uint32_t j = 0; j < 800; j++) { }
    }
}

int sht31_read(int32_t *temp_x100, int32_t *humidity_x100) {
    /* 1. Send the measurement command (2 bytes, MSB first). */
    uint8_t cmd[2] = { (MEAS_HIGH_REP >> 8) & 0xFF,
                        MEAS_HIGH_REP       & 0xFF };
    if (i2c_write(SHT31_ADDR, cmd, 2) < 0) return -1;

    /* 2. Wait for the conversion. Datasheet says high-repetability mode
     *    takes max 15 ms. We use 20 to be safe. */
    delay_ms(20);

    /* 3. Read 6 bytes: T_msb, T_lsb, T_crc, H_msb, H_lsb, H_crc. */
    uint8_t buf[6];
    if (i2c_read(SHT31_ADDR, buf, 6) < 0) return -2;

    /* 4. CRC-check both halves before trusting them. */
    if (sht31_crc8(&buf[0], 2) != buf[2]) return -3;
    if (sht31_crc8(&buf[3], 2) != buf[5]) return -4;

    /* 5. Reassemble the 16-bit raw values. */
    uint16_t raw_t = (uint16_t)((buf[0] << 8) | buf[1]);
    uint16_t raw_h = (uint16_t)((buf[3] << 8) | buf[4]);

    /* 6. Apply the conversion formulas from the SHT3x datasheet, scaled
     *    by 100 to keep everything integer.
     *
     *    T (°C) = -45 + 175 × raw / 65535
     *    Multiplying by 100:
     *    T_x100 = -4500 + 17500 × raw / 65535
     *
     *    Use 32-bit math; (17500 × 65535) fits in uint32_t. */
    *temp_x100     = -4500 + (int32_t)((17500U * (uint32_t)raw_t) / 65535U);
    *humidity_x100 =         (int32_t)((10000U * (uint32_t)raw_h) / 65535U);
    return 0;
}
