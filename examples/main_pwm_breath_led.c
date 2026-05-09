#include <stdint.h>

#include "stm32f1_gpio.h"

#define LED_PIN 0u

#define RCC_APB1ENR        (*(volatile uint32_t *)0x4002101Cu)
#define RCC_APB1ENR_TIM2EN (1u << 0)

#define TIM2_CR1   (*(volatile uint32_t *)0x40000000u)
#define TIM2_EGR   (*(volatile uint32_t *)0x40000014u)
#define TIM2_CCMR1 (*(volatile uint32_t *)0x40000018u)
#define TIM2_CCER  (*(volatile uint32_t *)0x40000020u)
#define TIM2_PSC   (*(volatile uint32_t *)0x40000028u)
#define TIM2_ARR   (*(volatile uint32_t *)0x4000002Cu)
#define TIM2_CCR1  (*(volatile uint32_t *)0x40000034u)

#define TIM_CR1_CEN  (1u << 0)
#define TIM_EGR_UG   (1u << 0)
#define TIM_CCMR1_OC1PE (1u << 3)
#define TIM_CCMR1_OC1M_PWM1 (6u << 4)
#define TIM_CCER_CC1E (1u << 0)
#define TIM_CCER_CC1P (1u << 1)

#define PWM_TOP 999u
#define BREATH_STEP 5u

static void delay(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm volatile ("nop");
    }
}

static void tim2_pwm_ch1_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2_CR1 = 0u;
    TIM2_PSC = 7u;       /* 8 MHz / (7 + 1) = 1 MHz timer clock. */
    TIM2_ARR = PWM_TOP;  /* 1 MHz / (999 + 1) = 1 kHz PWM. */
    TIM2_CCR1 = 0u;

    TIM2_CCMR1 &= ~(0xFFu << 0u);
    TIM2_CCMR1 |= TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;

    /*
     * LED anode is on PA0 and cathode is tied to GND.
     * Non-inverted CH1 makes a larger CCR1 produce a longer high-level interval,
     * so a larger duty value means a brighter LED.
     */
    TIM2_CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1P);
    TIM2_CCER |= TIM_CCER_CC1E;

    TIM2_EGR = TIM_EGR_UG;
    TIM2_CR1 = TIM_CR1_CEN;
}

int main(void)
{
    uint32_t duty = 0u;
    int32_t direction = 1;

    RCC_EnableGPIOA();
    GPIO_ConfigAlternatePushPull(GPIOA, LED_PIN);
    tim2_pwm_ch1_init();

    while (1) {
        TIM2_CCR1 = duty;

        if (direction > 0) {
            duty += BREATH_STEP;
            if (duty >= PWM_TOP) {
                duty = PWM_TOP;
                direction = -1;
            }
        } else {
            if (duty <= BREATH_STEP) {
                duty = 0u;
                direction = 1;
            } else {
                duty -= BREATH_STEP;
            }
        }

        delay(2000u);
    }
}
