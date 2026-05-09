#include <stdint.h>

#include "stm32f1_gpio.h"

#define SERVO_PIN  1u
#define BUTTON_PIN 1u

#define RCC_APB1ENR        (*(volatile uint32_t *)0x4002101Cu)
#define RCC_APB1ENR_TIM2EN (1u << 0)

#define TIM2_CR1   (*(volatile uint32_t *)0x40000000u)
#define TIM2_EGR   (*(volatile uint32_t *)0x40000014u)
#define TIM2_CCMR1 (*(volatile uint32_t *)0x40000018u)
#define TIM2_CCER  (*(volatile uint32_t *)0x40000020u)
#define TIM2_PSC   (*(volatile uint32_t *)0x40000028u)
#define TIM2_ARR   (*(volatile uint32_t *)0x4000002Cu)
#define TIM2_CCR2  (*(volatile uint32_t *)0x40000038u)

#define TIM_CR1_CEN          (1u << 0)
#define TIM_EGR_UG           (1u << 0)
#define TIM_CCMR1_OC2PE      (1u << 11)
#define TIM_CCMR1_OC2M_PWM1  (6u << 12)
#define TIM_CCER_CC2E        (1u << 4)
#define TIM_CCER_CC2P        (1u << 5)

#define SERVO_MIN_US  500u
#define SERVO_MAX_US  2500u
#define SERVO_STEP_DEG 30u
#define SERVO_MAX_DEG 180u

static void delay(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm volatile ("nop");
    }
}

static void tim2_pwm_ch2_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2_CR1 = 0u;
    TIM2_PSC = 7u;          /* 8 MHz / (7 + 1) = 1 MHz, 1 count = 1 us. */
    TIM2_ARR = 20000u - 1u; /* 20 ms period, 50 Hz servo control pulse. */
    TIM2_CCR2 = SERVO_MIN_US;

    TIM2_CCMR1 &= ~(0xFFu << 8u);
    TIM2_CCMR1 |= TIM_CCMR1_OC2M_PWM1 | TIM_CCMR1_OC2PE;

    TIM2_CCER &= ~(TIM_CCER_CC2E | TIM_CCER_CC2P);
    TIM2_CCER |= TIM_CCER_CC2E;

    TIM2_EGR = TIM_EGR_UG;
    TIM2_CR1 = TIM_CR1_CEN;
}

static uint32_t servo_pulse_us(uint32_t angle_deg)
{
    return SERVO_MIN_US + ((SERVO_MAX_US - SERVO_MIN_US) * angle_deg) / SERVO_MAX_DEG;
}

static uint8_t button_is_pressed(void)
{
    return GPIO_ReadPin(GPIOB, BUTTON_PIN) == GPIO_PIN_RESET;
}

int main(void)
{
    uint32_t angle = 0u;
    uint8_t was_pressed = 0u;

    RCC_EnableGPIOA();
    RCC_EnableGPIOB();
    GPIO_ConfigAlternatePushPull(GPIOA, SERVO_PIN);
    GPIO_ConfigInputPullUp(GPIOB, BUTTON_PIN);
    tim2_pwm_ch2_init();

    TIM2_CCR2 = servo_pulse_us(angle);

    while (1) {
        uint8_t pressed = button_is_pressed();

        if (pressed && !was_pressed) {
            delay(50000u);
            if (button_is_pressed()) {
                angle += SERVO_STEP_DEG;
                if (angle > SERVO_MAX_DEG) {
                    angle = 0u;
                }
                TIM2_CCR2 = servo_pulse_us(angle);
                was_pressed = 1u;
            }
        } else if (!pressed) {
            was_pressed = 0u;
        }

        delay(2000u);
    }
}
