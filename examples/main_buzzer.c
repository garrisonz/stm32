#include <stdint.h>

#include "stm32f1_gpio.h"

#define BUZZER_PIN 12u
#define BEEP_PULSES 800u
#define ON_TIME     120u
#define OFF_TIME    480u
#define PAUSE_TIME  3000000u

static void delay(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm volatile ("nop");
    }
}

int main(void)
{
    RCC_EnableGPIOB();
    GPIO_ConfigOutputPushPull(GPIOB, BUZZER_PIN);
    GPIO_WritePin(GPIOB, BUZZER_PIN, GPIO_PIN_SET); /* Buzzer off: PB12 high. */

    while (1) {
        for (uint32_t pulse = 0; pulse < BEEP_PULSES; ++pulse) {
            GPIO_WritePin(GPIOB, BUZZER_PIN, GPIO_PIN_RESET); /* Buzzer on: low level. */
            delay(ON_TIME);
            GPIO_WritePin(GPIOB, BUZZER_PIN, GPIO_PIN_SET);
            delay(OFF_TIME);
        }

        GPIO_WritePin(GPIOB, BUZZER_PIN, GPIO_PIN_SET);
        delay(PAUSE_TIME);
    }
}
