#include "stm32f1_gpio.h"

#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018u)
#define RCC_APB2ENR_IOPAEN (1u << 2)

void RCC_EnableGPIOA(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN;
}

void GPIO_ConfigOutputPushPull(GPIO_TypeDef *gpio, uint32_t pin)
{
    volatile uint32_t *config = pin < 8u ? &gpio->CRL : &gpio->CRH;
    uint32_t shift = (pin % 8u) * 4u;

    *config &= ~(0xFu << shift);
    *config |=  (0x2u << shift); /* Output push-pull, 2 MHz. */
}

void GPIO_WritePin(GPIO_TypeDef *gpio, uint32_t pin, GPIO_PinState state)
{
    if (state == GPIO_PIN_SET) {
        gpio->BSRR = 1u << pin;
    } else {
        gpio->BRR = 1u << pin;
    }
}
