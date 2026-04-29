#ifndef STM32F1_GPIO_H
#define STM32F1_GPIO_H

#include <stdint.h>

typedef enum {
    GPIO_PIN_RESET = 0,
    GPIO_PIN_SET = 1,
} GPIO_PinState;

typedef struct {
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t BRR;
    volatile uint32_t LCKR;
} GPIO_TypeDef;

#define GPIOA ((GPIO_TypeDef *)0x40010800u)
#define GPIOB ((GPIO_TypeDef *)0x40010C00u)

void RCC_EnableGPIOA(void);
void RCC_EnableGPIOB(void);
void GPIO_ConfigOutputPushPull(GPIO_TypeDef *gpio, uint32_t pin);
void GPIO_ConfigAlternatePushPull(GPIO_TypeDef *gpio, uint32_t pin);
void GPIO_ConfigOutputOpenDrain(GPIO_TypeDef *gpio, uint32_t pin);
void GPIO_ConfigInputPullUp(GPIO_TypeDef *gpio, uint32_t pin);
void GPIO_WritePin(GPIO_TypeDef *gpio, uint32_t pin, GPIO_PinState state);
GPIO_PinState GPIO_ReadPin(GPIO_TypeDef *gpio, uint32_t pin);

#endif
