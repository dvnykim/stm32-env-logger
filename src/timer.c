#include "stm32l476xx.h"
#include "timer.h"

/* The actual storage for the flag declared `extern` in timer.h. */
volatile uint8_t sample_ready = 0;

void timer_init_2s(void) {
    /* 1. Enable TIM2 clock (TIM2 sits on APB1). */
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

    /* 2. Set prescaler and auto-reload for a 2-second period.
     *    PSC=3999 → tick rate = 4MHz / 4000 = 1 kHz (1 ms per tick).
     *    ARR=1999 → 2000 ticks per overflow → 2.0 seconds. */
    TIM2->PSC = 3999;
    TIM2->ARR = 1999;

    /* 3. Force the prescaler/auto-reload registers to reload from their
     *    shadow registers. Without this, the first cycle uses old values. */
    TIM2->EGR = TIM_EGR_UG;

    /* 4. The line above generates a spurious update event. Clear the
     *    flag so the very first ISR fires after a real 2 seconds. */
    TIM2->SR &= ~TIM_SR_UIF;

    /* 5. Enable update interrupt — the hardware now wants to call our ISR
     *    every time the counter rolls over. */
    TIM2->DIER |= TIM_DIER_UIE;

    /* 6. Tell the NVIC (Nested Vectored Interrupt Controller) to actually
     *    route TIM2's interrupt to the CPU. The CMSIS function handles
     *    the NVIC register write for us. */
    NVIC_EnableIRQ(TIM2_IRQn);

    /* 7. Start the counter. */
    TIM2->CR1 |= TIM_CR1_CEN;
}

/* This name is special — it matches a "weak symbol" in startup_stm32l476xx.s
 * that defaults to an infinite loop. By defining a function with this exact
 * name, the linker uses ours instead. The vector table entry for TIM2's
 * interrupt points here automatically. */
void TIM2_IRQHandler(void) {
    /* Always check WHY the interrupt fired (TIM2 has several interrupt
     * sources). We only enabled UIE, but defensive code never hurts. */
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;       /* clear the flag in hardware */
        sample_ready = 1;              /* signal the main loop */
    }
}
