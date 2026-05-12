#ifndef SHT31_H
#define SHT31_H

#include <stdint.h>

/* Read a single-shot temperature + humidity measurement.
 *
 * On success returns 0 and writes:
 *   *temp_x100      → temperature in 0.01 °C  (e.g. 2347 = 23.47 °C)
 *   *humidity_x100  → humidity in 0.01 %RH
 * Returns negative on I2C error or CRC mismatch. */
int sht31_read(int32_t *temp_x100, int32_t *humidity_x100);

#endif /* SHT31_H */
