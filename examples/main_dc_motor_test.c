#include <stdint.h>

#include "stm32f1_gpio.h"

#define PWMA_PIN 2u
#define AIN1_PIN 4u
#define AIN2_PIN 5u

#define RCC_APB1ENR        (*(volatile uint32_t *)0x4002101Cu)
#define RCC_APB1ENR_TIM2EN (1u << 0)

#define TIM2_CR1   (*(volatile uint32_t *)0x40000000u)
#define TIM2_EGR   (*(volatile uint32_t *)0x40000014u)
#define TIM2_CCMR2 (*(volatile uint32_t *)0x4000001Cu)
#define TIM2_CCER  (*(volatile uint32_t *)0x40000020u)
#define TIM2_PSC   (*(volatile uint32_t *)0x40000028u)
#define TIM2_ARR   (*(volatile uint32_t *)0x4000002Cu)
#define TIM2_CCR3  (*(volatile uint32_t *)0x4000003Cu)

#define TIM_CR1_CEN         (1u << 0)
#define TIM_EGR_UG          (1u << 0)
#define TIM_CCMR2_OC3PE     (1u << 3)
#define TIM_CCMR2_OC3M_PWM1 (6u << 4)
#define TIM_CCER_CC3E       (1u << 8)
#define TIM_CCER_CC3P       (1u << 9)

#define PWM_TOP 99u

static void tim2_pwm_ch3_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2_CR1 = 0u;
    TIM2_PSC = 79u;     /* 8 MHz / 80 = 100 kHz. */
    TIM2_ARR = PWM_TOP; /* 100 kHz / 100 = 1 kHz PWM, same scale as tutorial. */
    TIM2_CCR3 = 50u;    /* 50% duty. */

    TIM2_CCMR2 &= ~(0xFFu << 0u);
    TIM2_CCMR2 |= TIM_CCMR2_OC3M_PWM1 | TIM_CCMR2_OC3PE;

    TIM2_CCER &= ~(TIM_CCER_CC3E | TIM_CCER_CC3P);
    TIM2_CCER |= TIM_CCER_CC3E;

    TIM2_EGR = TIM_EGR_UG;
    TIM2_CR1 = TIM_CR1_CEN;
}

int main(void)
{
    RCC_EnableGPIOA();
    GPIO_ConfigAlternatePushPull(GPIOA, PWMA_PIN);
    GPIO_ConfigOutputPushPull(GPIOA, AIN1_PIN);
    GPIO_ConfigOutputPushPull(GPIOA, AIN2_PIN);

    GPIO_WritePin(GPIOA, AIN1_PIN, GPIO_PIN_SET);
    GPIO_WritePin(GPIOA, AIN2_PIN, GPIO_PIN_RESET);
    tim2_pwm_ch3_init();

    while (1) {
        __asm volatile ("nop");
    }
}
