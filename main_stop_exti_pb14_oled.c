#include <stdint.h>

#include "oled_display.h"
#include "stm32f1_gpio.h"

#define RCC_APB2ENR          (*(volatile uint32_t *)0x40021018u)
#define RCC_APB1ENR          (*(volatile uint32_t *)0x4002101Cu)
#define AFIO_EXTICR4         (*(volatile uint32_t *)0x40010014u)
#define EXTI_IMR             (*(volatile uint32_t *)0x40010400u)
#define EXTI_RTSR            (*(volatile uint32_t *)0x40010408u)
#define EXTI_FTSR            (*(volatile uint32_t *)0x4001040Cu)
#define EXTI_PR              (*(volatile uint32_t *)0x40010414u)
#define PWR_CR               (*(volatile uint32_t *)0x40007000u)
#define SCB_SCR              (*(volatile uint32_t *)0xE000ED10u)
#define NVIC_ISER1           (*(volatile uint32_t *)0xE000E104u)

#define SYSTICK_CSR          (*(volatile uint32_t *)0xE000E010u)
#define SYSTICK_RVR          (*(volatile uint32_t *)0xE000E014u)
#define SYSTICK_CVR          (*(volatile uint32_t *)0xE000E018u)

#define RCC_APB2ENR_AFIOEN   (1u << 0)
#define RCC_APB2ENR_IOPBEN   (1u << 3)
#define RCC_APB1ENR_PWREN    (1u << 28)

#define PWR_CR_LPDS          (1u << 0)
#define PWR_CR_PDDS          (1u << 1)
#define PWR_CR_CWUF          (1u << 2)
#define SCB_SCR_SLEEPDEEP    (1u << 2)

#define SENSOR_PIN           14u
#define EXTI14_LINE          (1u << 14)
#define EXTI15_10_IRQn       40u

static volatile uint32_t count_sensor_count;

static void delay_ms(uint32_t ms)
{
    SYSTICK_RVR = 8000u - 1u;
    SYSTICK_CVR = 0u;
    SYSTICK_CSR = 0x05u;

    while (ms-- > 0u) {
        while ((SYSTICK_CSR & (1u << 16)) == 0u) {
        }
    }

    SYSTICK_CSR = 0u;
}

static uint8_t count_sensor_is_low(void)
{
    return GPIO_ReadPin(GPIOB, SENSOR_PIN) == GPIO_PIN_RESET ? 1u : 0u;
}

static void count_sensor_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;

    GPIO_ConfigInputPullUp(GPIOB, SENSOR_PIN);

    AFIO_EXTICR4 &= ~(0xFu << 8u);
    AFIO_EXTICR4 |=  (0x1u << 8u); /* EXTI14 = PB14. */

    EXTI_IMR |= EXTI14_LINE;
    EXTI_RTSR |= EXTI14_LINE;
    EXTI_FTSR |= EXTI14_LINE;
    EXTI_PR = EXTI14_LINE;

    NVIC_ISER1 = 1u << (EXTI15_10_IRQn - 32u);
    __asm volatile ("cpsie i");
}

static void stop_mode_enter(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_PWREN;

    PWR_CR |= PWR_CR_CWUF;
    PWR_CR &= ~PWR_CR_PDDS;
    PWR_CR &= ~PWR_CR_LPDS;
    EXTI_PR = EXTI14_LINE;

    SCB_SCR |= SCB_SCR_SLEEPDEEP;
    __asm volatile ("wfi");
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
}

static void make_count_text(char *text, uint32_t count)
{
    text[0] = 'C';
    text[1] = 'o';
    text[2] = 'u';
    text[3] = 'n';
    text[4] = 't';
    text[5] = ':';
    text[6] = (char)('0' + ((count / 10000u) % 10u));
    text[7] = (char)('0' + ((count / 1000u) % 10u));
    text[8] = (char)('0' + ((count / 100u) % 10u));
    text[9] = (char)('0' + ((count / 10u) % 10u));
    text[10] = (char)('0' + (count % 10u));
    text[11] = '\0';
}

void EXTI15_10_IRQHandler(void)
{
    if ((EXTI_PR & EXTI14_LINE) != 0u) {
        if (count_sensor_is_low()) {
            count_sensor_count++;
        }
        EXTI_PR = EXTI14_LINE;
    }
}

int main(void)
{
    char count_text[12];

    OLED_DisplayUseSoftwareI2C(1u);
    OLED_DisplayInit();
    count_sensor_init();

    while (1) {
        make_count_text(count_text, count_sensor_count);
        OLED_DisplayTextStatus(count_text, "Running");
        delay_ms(100u);
        OLED_DisplayTextStatus(count_text, "       ");
        delay_ms(100u);

        OLED_DisplayTextStatus(count_text, "STOP WAIT");
        delay_ms(100u);

        stop_mode_enter();

        OLED_DisplayInit();
        count_sensor_init();
    }
}
