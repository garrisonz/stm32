#include <stdint.h>

#include "oled_display.h"
#include "stm32f1_gpio.h"

#define PWM_OUT_PIN 0u
#define IC_IN_PIN   6u

#define RCC_APB1ENR        (*(volatile uint32_t *)0x4002101Cu)
#define RCC_APB1ENR_TIM2EN (1u << 0)
#define RCC_APB1ENR_TIM3EN (1u << 1)

#define TIM2_CR1   (*(volatile uint32_t *)0x40000000u)
#define TIM2_EGR   (*(volatile uint32_t *)0x40000014u)
#define TIM2_CCMR1 (*(volatile uint32_t *)0x40000018u)
#define TIM2_CCER  (*(volatile uint32_t *)0x40000020u)
#define TIM2_PSC   (*(volatile uint32_t *)0x40000028u)
#define TIM2_ARR   (*(volatile uint32_t *)0x4000002Cu)
#define TIM2_CCR1  (*(volatile uint32_t *)0x40000034u)

#define TIM3_CR1   (*(volatile uint32_t *)0x40000400u)
#define TIM3_SMCR  (*(volatile uint32_t *)0x40000408u)
#define TIM3_SR    (*(volatile uint32_t *)0x40000410u)
#define TIM3_EGR   (*(volatile uint32_t *)0x40000414u)
#define TIM3_CCMR1 (*(volatile uint32_t *)0x40000418u)
#define TIM3_CCER  (*(volatile uint32_t *)0x40000420u)
#define TIM3_PSC   (*(volatile uint32_t *)0x40000428u)
#define TIM3_ARR   (*(volatile uint32_t *)0x4000042Cu)
#define TIM3_CCR1  (*(volatile uint32_t *)0x40000434u)
#define TIM3_CCR2  (*(volatile uint32_t *)0x40000438u)

#define TIM_CR1_CEN          (1u << 0)
#define TIM_EGR_UG           (1u << 0)
#define TIM_SR_CC1IF         (1u << 1)
#define TIM_SR_CC2IF         (1u << 2)
#define TIM_CCMR1_OC1PE      (1u << 3)
#define TIM_CCMR1_OC1M_PWM1  (6u << 4)
#define TIM_CCMR1_CC1S_TI1   (1u << 0)
#define TIM_CCMR1_CC2S_TI1   (2u << 8)
#define TIM_CCER_CC1E        (1u << 0)
#define TIM_CCER_CC1P        (1u << 1)
#define TIM_CCER_CC2E        (1u << 4)
#define TIM_CCER_CC2P        (1u << 5)
#define TIM_SMCR_SMS_RESET   (4u << 0)
#define TIM_SMCR_TS_TI1FP1   (5u << 4)

#define TIMER_CLOCK_HZ 1000000u
#define DISPLAY_EVERY_CAPTURES 200u

static void gpio_config_input_floating(GPIO_TypeDef *gpio, uint32_t pin)
{
    volatile uint32_t *config = pin < 8u ? &gpio->CRL : &gpio->CRH;
    uint32_t shift = (pin % 8u) * 4u;

    *config &= ~(0xFu << shift);
    *config |=  (0x4u << shift);
}

static void pwm_output_pa0_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    GPIO_ConfigAlternatePushPull(GPIOA, PWM_OUT_PIN);

    TIM2_CR1 = 0u;
    TIM2_PSC = 7u;      /* 8 MHz / 8 = 1 MHz timer clock. */
    TIM2_ARR = 99u;    /* 1 MHz / 1000 = 1 kHz PWM. */
    TIM2_CCR1 = 70u;   /* 50% duty. */

    TIM2_CCMR1 &= ~(0xFFu << 0u);
    TIM2_CCMR1 |= TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM2_CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1P);
    TIM2_CCER |= TIM_CCER_CC1E;

    TIM2_EGR = TIM_EGR_UG;
    TIM2_CR1 = TIM_CR1_CEN;
}

static void input_capture_pa6_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM3EN;

    gpio_config_input_floating(GPIOA, IC_IN_PIN);

    TIM3_CR1 = 0u;
    TIM3_PSC = 7u;       /* 8 MHz / 8 = 1 MHz, 1 count = 1 us. */
    TIM3_ARR = 0xFFFFu;

    TIM3_CCMR1 &= ~(0xFFu << 0u);
    TIM3_CCMR1 |= TIM_CCMR1_CC1S_TI1 | TIM_CCMR1_CC2S_TI1;

    TIM3_CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1P);
    TIM3_CCER |= TIM_CCER_CC1E;
    TIM3_CCER &= ~(TIM_CCER_CC2E | TIM_CCER_CC2P);
    TIM3_CCER |= TIM_CCER_CC2E | TIM_CCER_CC2P;

    TIM3_SMCR = TIM_SMCR_TS_TI1FP1 | TIM_SMCR_SMS_RESET;
    TIM3_EGR = TIM_EGR_UG;
    TIM3_SR = 0u;
    TIM3_CR1 = TIM_CR1_CEN;
}

static uint32_t capture_frequency_hz(void)
{
    uint32_t period_counts = TIM3_CCR1;

    if (period_counts == 0u) {
        return 0u;
    }

    return TIMER_CLOCK_HZ / (period_counts + 1u);
}

static uint32_t capture_duty_percent(void)
{
    uint32_t period_counts = TIM3_CCR1 + 1u;
    uint32_t high_counts = TIM3_CCR2;

    if (period_counts == 0u) {
        return 0u;
    }

    if (high_counts > period_counts) {
        high_counts = period_counts;
    }

    return (high_counts * 100u + (period_counts / 2u)) / period_counts;
}

int main(void)
{
    uint32_t latest_frequency = 0u;
    uint32_t latest_duty = 0u;
    uint32_t capture_count = 0u;

    RCC_EnableGPIOA();
    OLED_DisplayInit();
    OLED_DisplayMeasurement(0u, 0u);

    pwm_output_pa0_init();
    input_capture_pa6_init();

    while (1) {
        if ((TIM3_SR & TIM_SR_CC2IF) != 0u) {
            latest_frequency = capture_frequency_hz();
            latest_duty = capture_duty_percent();
            TIM3_SR = 0u;

            ++capture_count;
            if (capture_count >= DISPLAY_EVERY_CAPTURES) {
                OLED_DisplayMeasurement(latest_frequency, latest_duty);
                capture_count = 0u;
            }
        }
    }
}
