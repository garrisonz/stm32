#include <stdint.h>

#include "stm32f1_gpio.h"

#define LED1_PIN    1u
#define LED2_PIN    2u
#define BUTTON1_PIN 11u
#define BUTTON2_PIN 1u

#define DEBOUNCE_DELAY 30000u
#define LOOP_DELAY     2000u

static void delay(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm volatile ("nop");
    }
}

static void set_led(uint32_t pin, uint8_t on)
{
    GPIO_WritePin(GPIOA, pin, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static uint8_t button_pressed(uint32_t pin)
{
    return GPIO_ReadPin(GPIOB, pin) == GPIO_PIN_RESET;
}

int main(void)
{
    uint8_t led1_on = 0;
    uint8_t led2_on = 0;
    uint8_t button1_was_pressed = 0;
    uint8_t button2_was_pressed = 0;

    RCC_EnableGPIOA();
    RCC_EnableGPIOB();

    GPIO_ConfigOutputPushPull(GPIOA, LED1_PIN);
    GPIO_ConfigOutputPushPull(GPIOA, LED2_PIN);
    GPIO_ConfigInputPullUp(GPIOB, BUTTON1_PIN);
    GPIO_ConfigInputPullUp(GPIOB, BUTTON2_PIN);

    set_led(LED1_PIN, led1_on);
    set_led(LED2_PIN, led2_on);

    while (1) {
        uint8_t button1_is_pressed = button_pressed(BUTTON1_PIN);
        uint8_t button2_is_pressed = button_pressed(BUTTON2_PIN);

        if (button1_is_pressed && !button1_was_pressed) {
            delay(DEBOUNCE_DELAY);
            if (button_pressed(BUTTON1_PIN)) {
                led1_on = !led1_on;
                set_led(LED1_PIN, led1_on);
                button1_was_pressed = 1;
            }
        } else if (!button1_is_pressed) {
            button1_was_pressed = 0;
        }

        if (button2_is_pressed && !button2_was_pressed) {
            delay(DEBOUNCE_DELAY);
            if (button_pressed(BUTTON2_PIN)) {
                led2_on = !led2_on;
                set_led(LED2_PIN, led2_on);
                button2_was_pressed = 1;
            }
        } else if (!button2_is_pressed) {
            button2_was_pressed = 0;
        }

        delay(LOOP_DELAY);
    }
}
