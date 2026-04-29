#include <stdint.h>

#include "stm32f1_gpio.h"

#define PWMA_PIN   2u
#define AIN1_PIN   4u
#define AIN2_PIN   5u
#define BUTTON_PIN 1u

#define RCC_APB1ENR        (*(volatile uint32_t *)0x4002101Cu)
#define RCC_APB1ENR_TIM2EN (1u << 0)

#define TIM2_CR1   (*(volatile uint32_t *)0x40000000u)
#define TIM2_EGR   (*(volatile uint32_t *)0x40000014u)
#define TIM2_CCMR2 (*(volatile uint32_t *)0x4000001Cu)
#define TIM2_CCER  (*(volatile uint32_t *)0x40000020u)
#define TIM2_PSC   (*(volatile uint32_t *)0x40000028u)
#define TIM2_ARR   (*(volatile uint32_t *)0x4000002Cu)
#define TIM2_CCR3  (*(volatile uint32_t *)0x4000003Cu)

#define TIM_CR1_CEN          (1u << 0)
#define TIM_EGR_UG           (1u << 0)
#define TIM_CCMR2_OC3PE      (1u << 3)
#define TIM_CCMR2_OC3M_PWM1  (6u << 4)
#define TIM_CCER_CC3E        (1u << 8)
#define TIM_CCER_CC3P        (1u << 9)

#define PWM_TOP 399u
#define SPEED_STEP_PERCENT 10u
#define SPEED_MAX_PERCENT 100u

static void delay(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm volatile ("nop");
    }
}

static void tim2_pwm_ch3_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2_CR1 = 0u;
    TIM2_PSC = 0u;      /* 8 MHz timer clock. */
    TIM2_ARR = PWM_TOP; /* 8 MHz / 400 = 20 kHz PWM. */
    TIM2_CCR3 = 0u;

    TIM2_CCMR2 &= ~(0xFFu << 0u);
    TIM2_CCMR2 |= TIM_CCMR2_OC3M_PWM1 | TIM_CCMR2_OC3PE;

    TIM2_CCER &= ~(TIM_CCER_CC3E | TIM_CCER_CC3P);
    TIM2_CCER |= TIM_CCER_CC3E;

    TIM2_EGR = TIM_EGR_UG;
    TIM2_CR1 = TIM_CR1_CEN;
}

static void motor_set_speed_percent(uint32_t percent)
{
    if (percent >= SPEED_MAX_PERCENT) {
        TIM2_CCR3 = PWM_TOP + 1u;
    } else {
        TIM2_CCR3 = ((PWM_TOP + 1u) * percent) / SPEED_MAX_PERCENT;
    }
}

static void motor_stop(void)
{
    TIM2_CCR3 = 0u;
    GPIO_WritePin(GPIOA, AIN1_PIN, GPIO_PIN_RESET);
    GPIO_WritePin(GPIOA, AIN2_PIN, GPIO_PIN_RESET);
}

static void motor_set_speed(int32_t speed_percent)
{
    if (speed_percent > 0) {
        GPIO_WritePin(GPIOA, AIN1_PIN, GPIO_PIN_SET);
        GPIO_WritePin(GPIOA, AIN2_PIN, GPIO_PIN_RESET);
        motor_set_speed_percent((uint32_t)speed_percent);
    } else if (speed_percent < 0) {
        GPIO_WritePin(GPIOA, AIN1_PIN, GPIO_PIN_RESET);
        GPIO_WritePin(GPIOA, AIN2_PIN, GPIO_PIN_SET);
        motor_set_speed_percent((uint32_t)(-speed_percent));
    } else {
        motor_stop();
    }
}

static uint8_t button_is_pressed(void)
{
    return GPIO_ReadPin(GPIOB, BUTTON_PIN) == GPIO_PIN_RESET;
}

int main(void)
{
    int32_t speed_percent = 0;
    int32_t direction = 1;
    uint8_t was_pressed = 0u;

    RCC_EnableGPIOA();
    RCC_EnableGPIOB();
    GPIO_ConfigAlternatePushPull(GPIOA, PWMA_PIN);
    GPIO_ConfigOutputPushPull(GPIOA, AIN1_PIN);
    GPIO_ConfigOutputPushPull(GPIOA, AIN2_PIN);
    GPIO_ConfigInputPullUp(GPIOB, BUTTON_PIN);
    tim2_pwm_ch3_init();
    motor_stop();

    while (1) {
        uint8_t pressed = button_is_pressed();

        if (pressed && !was_pressed) {
            delay(50000u);
            if (button_is_pressed()) {
                if (speed_percent == 0) {
                    speed_percent = direction * (int32_t)SPEED_STEP_PERCENT;
                } else if (speed_percent > 0) {
                    speed_percent += (int32_t)SPEED_STEP_PERCENT;
                    if (speed_percent > (int32_t)SPEED_MAX_PERCENT) {
                        speed_percent = 0;
                        direction = -1;
                    }
                } else {
                    speed_percent -= (int32_t)SPEED_STEP_PERCENT;
                    if (speed_percent < -(int32_t)SPEED_MAX_PERCENT) {
                        speed_percent = 0;
                        direction = 1;
                    }
                }
                motor_set_speed(speed_percent);
                was_pressed = 1u;
            }
        } else if (!pressed) {
            was_pressed = 0u;
        }

        delay(2000u);
    }
}
