#include <stdint.h>

#include "stm32f1_gpio.h"

#define LED_FIRST_PIN 0u
#define LED_COUNT     8u

int main(void)
{
    RCC_EnableGPIOA();

    for (uint32_t pin = LED_FIRST_PIN; pin < LED_FIRST_PIN + LED_COUNT; ++pin) {
        GPIO_ConfigOutputPushPull(GPIOA, pin);
        GPIO_WritePin(GPIOA, pin, GPIO_PIN_SET); /* LED off: PAx high. */
    }

    while (1) {
        __asm volatile ("nop");
    }
}
