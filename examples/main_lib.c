#include <stdint.h>

#include "stm32f1_gpio.h"

#define LED_FIRST_PIN 0u
#define LED_COUNT     8u
#define LED_DELAY     250000u

static void delay(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm volatile ("nop");
    }
}

int main(void)
{
    RCC_EnableGPIOA();

    for (uint32_t pin = LED_FIRST_PIN; pin < LED_FIRST_PIN + LED_COUNT; ++pin) {
        GPIO_ConfigOutputPushPull(GPIOA, pin);
        GPIO_WritePin(GPIOA, pin, GPIO_PIN_SET); /* LED off: pins idle high. */
    }

    while (1) {
        for (uint32_t pin = LED_FIRST_PIN; pin < LED_FIRST_PIN + LED_COUNT; ++pin) {
            GPIO_WritePin(GPIOA, pin, GPIO_PIN_RESET); /* LED on: PAx sinks current. */
            delay(LED_DELAY);
            GPIO_WritePin(GPIOA, pin, GPIO_PIN_SET);
        }
    }
}
