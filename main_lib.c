#include <stdint.h>

#include "stm32f1_gpio.h"

#define LED_PIN 0u

static void delay(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm volatile ("nop");
    }
}

int main(void)
{
    RCC_EnableGPIOA();
    GPIO_ConfigOutputPushPull(GPIOA, LED_PIN);

    while (1) {
        GPIO_WritePin(GPIOA, LED_PIN, GPIO_PIN_RESET); /* LED on: PA0 sinks current. */
        delay(1000000u);
        GPIO_WritePin(GPIOA, LED_PIN, GPIO_PIN_SET);   /* LED off. */
        delay(1000000u);
    }
}
