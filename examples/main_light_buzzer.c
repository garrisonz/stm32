#include <stdint.h>

#include "stm32f1_gpio.h"

#define BUZZER_PIN 12u
#define LIGHT_PIN  13u

#define DARK_LEVEL GPIO_PIN_SET

#define BUZZER_ON_TIME  300u
#define BUZZER_OFF_TIME 300u
#define SENSOR_DELAY    2000u

static void delay(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm volatile ("nop");
    }
}

static uint8_t is_dark(void)
{
    return GPIO_ReadPin(GPIOB, LIGHT_PIN) == DARK_LEVEL;
}

int main(void)
{
    RCC_EnableGPIOB();

    GPIO_ConfigOutputPushPull(GPIOB, BUZZER_PIN);
    GPIO_ConfigInputPullUp(GPIOB, LIGHT_PIN);
    GPIO_WritePin(GPIOB, BUZZER_PIN, GPIO_PIN_SET); /* Buzzer off: PB12 high. */

    while (1) {
        if (is_dark()) {
            GPIO_WritePin(GPIOB, BUZZER_PIN, GPIO_PIN_RESET);
            delay(BUZZER_ON_TIME);
            GPIO_WritePin(GPIOB, BUZZER_PIN, GPIO_PIN_SET);
            delay(BUZZER_OFF_TIME);
        } else {
            GPIO_WritePin(GPIOB, BUZZER_PIN, GPIO_PIN_SET);
            delay(SENSOR_DELAY);
        }
    }
}
