#include <stdint.h>

#include "oled_display.h"
#include "stm32f1_gpio.h"

#define RCC_APB1ENR          (*(volatile uint32_t *)0x4002101Cu)
#define RCC_APB2ENR          (*(volatile uint32_t *)0x40021018u)
#define PWR_CR               (*(volatile uint32_t *)0x40007000u)
#define PWR_CSR              (*(volatile uint32_t *)0x40007004u)
#define BKP_DR1              (*(volatile uint32_t *)0x40006C04u)
#define BKP_DR2              (*(volatile uint32_t *)0x40006C08u)
#define SCB_SCR              (*(volatile uint32_t *)0xE000ED10u)

#define SYSTICK_CSR          (*(volatile uint32_t *)0xE000E010u)
#define SYSTICK_RVR          (*(volatile uint32_t *)0xE000E014u)
#define SYSTICK_CVR          (*(volatile uint32_t *)0xE000E018u)

#define RCC_APB1ENR_BKPEN    (1u << 27)
#define RCC_APB1ENR_PWREN    (1u << 28)
#define RCC_APB2ENR_IOPAEN   (1u << 2)

#define PWR_CR_LPDS          (1u << 0)
#define PWR_CR_PDDS          (1u << 1)
#define PWR_CR_CWUF          (1u << 2)
#define PWR_CR_CSBF          (1u << 3)
#define PWR_CR_DBP           (1u << 8)
#define PWR_CSR_WUF          (1u << 0)
#define PWR_CSR_SBF          (1u << 1)
#define PWR_CSR_EWUP         (1u << 8)

#define SCB_SCR_SLEEPDEEP    (1u << 2)
#define BKP_SIGNATURE        0x5A5Au
#define WKUP_PIN             0u

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

static void backup_domain_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
    PWR_CR |= PWR_CR_DBP;
}

static uint8_t booted_from_standby(void)
{
    return (PWR_CSR & PWR_CSR_SBF) != 0u ? 1u : 0u;
}

static uint16_t wake_count_load(uint8_t from_standby)
{
    uint16_t count = 0u;

    if ((BKP_DR2 & 0xFFFFu) == BKP_SIGNATURE) {
        count = (uint16_t)(BKP_DR1 & 0xFFFFu);
    } else {
        BKP_DR2 = BKP_SIGNATURE;
        BKP_DR1 = 0u;
    }

    if (from_standby) {
        count = (uint16_t)(count + 1u);
        BKP_DR1 = count;
    }

    return count;
}

static void make_wake_text(char *text, uint16_t count)
{
    text[0] = 'W';
    text[1] = 'A';
    text[2] = 'K';
    text[3] = 'E';
    text[4] = ':';
    text[5] = (char)('0' + ((count / 10000u) % 10u));
    text[6] = (char)('0' + ((count / 1000u) % 10u));
    text[7] = (char)('0' + ((count / 100u) % 10u));
    text[8] = (char)('0' + ((count / 10u) % 10u));
    text[9] = (char)('0' + (count % 10u));
    text[10] = '\0';
}

static void wkup_pin_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN;
    GPIO_ConfigInputPullDown(GPIOA, WKUP_PIN);
}

static void standby_mode_enter(void)
{
    PWR_CR |= PWR_CR_CWUF | PWR_CR_CSBF;
    PWR_CSR |= PWR_CSR_EWUP;
    PWR_CR &= ~PWR_CR_LPDS;
    PWR_CR |= PWR_CR_PDDS;

    SCB_SCR |= SCB_SCR_SLEEPDEEP;
    __asm volatile ("wfi");

    while (1) {
    }
}

int main(void)
{
    uint8_t from_standby;
    uint16_t wake_count;
    char wake_text[11];

    OLED_DisplayUseSoftwareI2C(1u);
    OLED_DisplayInit();
    backup_domain_init();
    wkup_pin_init();

    from_standby = booted_from_standby();
    wake_count = wake_count_load(from_standby);
    make_wake_text(wake_text, wake_count);

    if (from_standby) {
        OLED_DisplayTextStatus("STANDBY WAKE", wake_text);
    } else {
        OLED_DisplayTextStatus("STANDBY DEMO", wake_text);
    }

    PWR_CR |= PWR_CR_CWUF | PWR_CR_CSBF;
    delay_ms(2000u);

    OLED_DisplayTextStatus("ENTER STANDBY", "PA0 RISING");
    delay_ms(1000u);

    standby_mode_enter();
}
