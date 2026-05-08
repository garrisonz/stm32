#include "stm32f1_gpio.h"

#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018u)
#define RCC_APB2ENR_IOPAEN (1u << 2)
#define RCC_APB2ENR_IOPBEN (1u << 3)

void RCC_EnableGPIOA(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN;
}

void RCC_EnableGPIOB(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPBEN;
}

void GPIO_ConfigOutputPushPull(GPIO_TypeDef *gpio, uint32_t pin)
{
    volatile uint32_t *config = pin < 8u ? &gpio->CRL : &gpio->CRH;
    uint32_t shift = (pin % 8u) * 4u;

    *config &= ~(0xFu << shift);
    *config |=  (0x2u << shift); /* Output push-pull, 2 MHz. */
}

void GPIO_ConfigAlternatePushPull(GPIO_TypeDef *gpio, uint32_t pin)
{
    volatile uint32_t *config = pin < 8u ? &gpio->CRL : &gpio->CRH;
    uint32_t shift = (pin % 8u) * 4u;

    *config &= ~(0xFu << shift);
    *config |=  (0xAu << shift); /* Alternate-function push-pull, 2 MHz. */
}

void GPIO_ConfigOutputOpenDrain(GPIO_TypeDef *gpio, uint32_t pin)
{
    volatile uint32_t *config = pin < 8u ? &gpio->CRL : &gpio->CRH;
    uint32_t shift = (pin % 8u) * 4u;

    *config &= ~(0xFu << shift);
    *config |=  (0x6u << shift); /* Output open-drain, 2 MHz. */
}

void GPIO_ConfigInputPullUp(GPIO_TypeDef *gpio, uint32_t pin)
{
    volatile uint32_t *config = pin < 8u ? &gpio->CRL : &gpio->CRH;
    uint32_t shift = (pin % 8u) * 4u;

    *config &= ~(0xFu << shift);
    *config |=  (0x8u << shift); /* Input with pull-up/pull-down. */
    gpio->BSRR = 1u << pin;      /* ODR=1 selects pull-up on STM32F1. */
}

void GPIO_ConfigInputPullDown(GPIO_TypeDef *gpio, uint32_t pin)
{
    volatile uint32_t *config = pin < 8u ? &gpio->CRL : &gpio->CRH;
    uint32_t shift = (pin % 8u) * 4u;

    *config &= ~(0xFu << shift);
    *config |=  (0x8u << shift); /* Input with pull-up/pull-down. */
    gpio->BRR = 1u << pin;       /* ODR=0 selects pull-down on STM32F1. */
}

void GPIO_WritePin(GPIO_TypeDef *gpio, uint32_t pin, GPIO_PinState state)
{
    if (state == GPIO_PIN_SET) {
        gpio->BSRR = 1u << pin;
    } else {
        gpio->BRR = 1u << pin;
    }
}

GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *gpio, uint32_t pin)
{
    return (gpio->IDR & (1u << pin)) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}
