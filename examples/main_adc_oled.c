#include <stdint.h>

#include "oled_display.h"
#include "stm32f1_gpio.h"

#define ADC_INPUT_PIN 0u

#define RCC_APB2ENR         (*(volatile uint32_t *)0x40021018u)
#define RCC_CFGR            (*(volatile uint32_t *)0x40021004u)
#define RCC_APB2ENR_ADC1EN  (1u << 9)
#define RCC_CFGR_ADCPRE_Msk (3u << 14)
#define RCC_CFGR_ADCPRE_DIV6 (2u << 14)

#define ADC1_SR     (*(volatile uint32_t *)0x40012400u)
#define ADC1_CR1    (*(volatile uint32_t *)0x40012404u)
#define ADC1_CR2    (*(volatile uint32_t *)0x40012408u)
#define ADC1_SMPR2  (*(volatile uint32_t *)0x40012410u)
#define ADC1_SQR1   (*(volatile uint32_t *)0x4001242Cu)
#define ADC1_SQR3   (*(volatile uint32_t *)0x40012434u)
#define ADC1_DR     (*(volatile uint32_t *)0x4001244Cu)

#define ADC_SR_EOC          (1u << 1)
#define ADC_CR2_ADON        (1u << 0)
#define ADC_CR2_CAL         (1u << 2)
#define ADC_CR2_RSTCAL      (1u << 3)
#define ADC_CR2_EXTTRIG     (1u << 20)
#define ADC_CR2_SWSTART     (1u << 22)
#define ADC_CR2_EXTSEL_SWSTART (7u << 17)

#define ADC_MAX_VALUE 4095u
#define VREF_MV       3300u

static void delay(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm volatile ("nop");
    }
}

static void gpio_config_analog(GPIO_TypeDef *gpio, uint32_t pin)
{
    volatile uint32_t *config = pin < 8u ? &gpio->CRL : &gpio->CRH;
    uint32_t shift = (pin % 8u) * 4u;

    *config &= ~(0xFu << shift);
}

static void adc1_pa0_init(void)
{
    RCC_EnableGPIOA();
    RCC_APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_ADCPRE_Msk) | RCC_CFGR_ADCPRE_DIV6;

    gpio_config_analog(GPIOA, ADC_INPUT_PIN);

    ADC1_CR1 = 0u;
    ADC1_CR2 = 0u;
    ADC1_SMPR2 &= ~(7u << 0u);
    ADC1_SMPR2 |= 5u << 0u; /* ADC1_IN0 sample time: 55.5 cycles. */
    ADC1_SQR1 = 0u;
    ADC1_SQR3 = 0u;

    ADC1_CR2 = ADC_CR2_ADON;
    delay(1000u);

    ADC1_CR2 |= ADC_CR2_RSTCAL;
    while ((ADC1_CR2 & ADC_CR2_RSTCAL) != 0u) {
    }

    ADC1_CR2 |= ADC_CR2_CAL;
    while ((ADC1_CR2 & ADC_CR2_CAL) != 0u) {
    }

    ADC1_CR2 |= ADC_CR2_EXTSEL_SWSTART | ADC_CR2_EXTTRIG;
}

static uint16_t adc1_read_pa0(void)
{
    ADC1_SR = 0u;
    ADC1_CR2 |= ADC_CR2_SWSTART;

    while ((ADC1_SR & ADC_SR_EOC) == 0u) {
    }

    return (uint16_t)(ADC1_DR & 0xFFFFu);
}

static uint32_t adc_to_millivolts(uint16_t adc_value)
{
    return ((uint32_t)adc_value * VREF_MV + (ADC_MAX_VALUE / 2u)) / ADC_MAX_VALUE;
}

int main(void)
{
    adc1_pa0_init();
    OLED_DisplayInit();
    OLED_DisplayAdcVoltage(0u, 0u);

    while (1) {
        uint16_t adc_value = adc1_read_pa0();
        uint32_t millivolts = adc_to_millivolts(adc_value);

        OLED_DisplayAdcVoltage(adc_value, millivolts);
        delay(800000u);
    }
}
