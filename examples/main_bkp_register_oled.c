#include <stdint.h>

#include "oled_display.h"

#define RCC_APB1ENR          (*(volatile uint32_t *)0x4002101Cu)
#define PWR_CR               (*(volatile uint32_t *)0x40007000u)
#define BKP_DR1              (*(volatile uint32_t *)0x40006C04u)
#define BKP_DR2              (*(volatile uint32_t *)0x40006C08u)

#define RCC_APB1ENR_PWREN    (1u << 28)
#define RCC_APB1ENR_BKPEN    (1u << 27)
#define PWR_CR_DBP           (1u << 8)

#define BKP_SIGNATURE        0xBEEFu

#define SYSTICK_CSR          (*(volatile uint32_t *)0xE000E010u)
#define SYSTICK_RVR          (*(volatile uint32_t *)0xE000E014u)
#define SYSTICK_CVR          (*(volatile uint32_t *)0xE000E018u)

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

static void bkp_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
    PWR_CR |= PWR_CR_DBP;
}

static uint16_t bkp_read_dr1(void)
{
    return (uint16_t)(BKP_DR1 & 0xFFFFu);
}

static uint16_t bkp_read_dr2(void)
{
    return (uint16_t)(BKP_DR2 & 0xFFFFu);
}

static void bkp_write_dr1(uint16_t value)
{
    BKP_DR1 = value;
}

static void bkp_write_dr2(uint16_t value)
{
    BKP_DR2 = value;
}

int main(void)
{
    uint16_t previous;
    uint16_t current;
    uint16_t signature;
    uint8_t pass;

    OLED_DisplayInit();
    OLED_DisplayTextStatus("BKP", "INIT");

    bkp_init();

    previous = bkp_read_dr1();
    signature = bkp_read_dr2();

    if (signature != BKP_SIGNATURE) {
        previous = 0u;
        bkp_write_dr2(BKP_SIGNATURE);
    }

    current = (uint16_t)(previous + 1u);
    bkp_write_dr1(current);

    pass = (uint8_t)((bkp_read_dr1() == current) &&
                     (bkp_read_dr2() == BKP_SIGNATURE));

    OLED_DisplayBkpTest(previous, current, bkp_read_dr2(), pass);

    while (1) {
        delay_ms(1000u);
    }
}
