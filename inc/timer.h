#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* Configure TIM2 to fire an update interrupt every 2 seconds.
 * Assumes the timer clock is 4 MHz (the default with MSI clock). */
void timer_init_2s(void);

/* Flag set by the TIM2 ISR. Main loop polls this, then clears it.
 * MUST be volatile — the compiler doesn't know an interrupt can change it. */
extern volatile uint8_t sample_ready;

#endif /* TIMER_H */
