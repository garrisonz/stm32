#include <stdint.h>

#include "oled_display.h"

#define RCC_CR               (*(volatile uint32_t *)0x40021000u)
#define RCC_CFGR             (*(volatile uint32_t *)0x40021004u)
#define FLASH_ACR            (*(volatile uint32_t *)0x40022000u)

#define SYSTICK_CSR          (*(volatile uint32_t *)0xE000E010u)
#define SYSTICK_RVR          (*(volatile uint32_t *)0xE000E014u)
#define SYSTICK_CVR          (*(volatile uint32_t *)0xE000E018u)

#define RCC_CR_HSION         (1u << 0)
#define RCC_CR_HSIRDY        (1u << 1)
#define RCC_CR_PLLON         (1u << 24)
#define RCC_CR_PLLRDY        (1u << 25)

#define RCC_CFGR_SW_MASK     (3u << 0)
#define RCC_CFGR_SW_HSI      (0u << 0)
#define RCC_CFGR_SW_PLL      (2u << 0)
#define RCC_CFGR_SWS_MASK    (3u << 2)
#define RCC_CFGR_SWS_HSI     (0u << 2)
#define RCC_CFGR_SWS_PLL     (2u << 2)
#define RCC_CFGR_HPRE_MASK   (0xFu << 4)
#define RCC_CFGR_PPRE1_MASK  (7u << 8)
#define RCC_CFGR_PPRE2_MASK  (7u << 11)
#define RCC_CFGR_PPRE1_DIV2  (4u << 8)
#define RCC_CFGR_PLLSRC_HSI2 (0u << 16)
#define RCC_CFGR_PLLXTPRE    (1u << 17)
#define RCC_CFGR_PLLMUL_MASK (0xFu << 18)

#define FLASH_ACR_LATENCY_MASK (7u << 0)
#define FLASH_ACR_PRFTBE       (1u << 4)

#define SYSCLK_HSI_HZ       8000000u

typedef struct {
    uint32_t sysclk_hz;
    uint32_t pllmul_bits;
    uint32_t use_pll;
    uint32_t apb1_div2;
} ClockProfile;

static const ClockProfile clock_profiles[] = {
    {  8000000u, 0u,        0u, 0u },
    { 16000000u, 2u << 18,  1u, 0u },
    { 32000000u, 6u << 18,  1u, 0u },
    { 64000000u, 14u << 18, 1u, 1u },
};

static uint32_t current_sysclk_hz = SYSCLK_HSI_HZ;

static void systick_delay_ms(uint32_t ms)
{
    SYSTICK_RVR = (current_sysclk_hz / 1000u) - 1u;
    SYSTICK_CVR = 0u;
    SYSTICK_CSR = 0x05u;

    while (ms-- > 0u) {
        while ((SYSTICK_CSR & (1u << 16)) == 0u) {
        }
    }

    SYSTICK_CSR = 0u;
}

static void wait_hsi_ready(void)
{
    RCC_CR |= RCC_CR_HSION;
    while ((RCC_CR & RCC_CR_HSIRDY) == 0u) {
    }
}

static void switch_sysclk_to_hsi(void)
{
    wait_hsi_ready();
    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_HSI;
    while ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_HSI) {
    }
    current_sysclk_hz = SYSCLK_HSI_HZ;
}

static void set_flash_latency(uint32_t sysclk_hz)
{
    uint32_t latency = 0u;

    if (sysclk_hz > 48000000u) {
        latency = 2u;
    } else if (sysclk_hz > 24000000u) {
        latency = 1u;
    }

    FLASH_ACR = (FLASH_ACR & ~FLASH_ACR_LATENCY_MASK) |
                FLASH_ACR_PRFTBE | latency;
}

static void clock_apply_profile(const ClockProfile *profile)
{
    switch_sysclk_to_hsi();

    RCC_CR &= ~RCC_CR_PLLON;
    while ((RCC_CR & RCC_CR_PLLRDY) != 0u) {
    }

    set_flash_latency(profile->sysclk_hz);

    RCC_CFGR &= ~(RCC_CFGR_HPRE_MASK | RCC_CFGR_PPRE1_MASK |
                  RCC_CFGR_PPRE2_MASK | RCC_CFGR_PLLSRC_HSI2 |
                  RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMUL_MASK);

    if (profile->apb1_div2) {
        RCC_CFGR |= RCC_CFGR_PPRE1_DIV2;
    }

    if (!profile->use_pll) {
        current_sysclk_hz = SYSCLK_HSI_HZ;
        return;
    }

    RCC_CFGR |= profile->pllmul_bits;
    RCC_CR |= RCC_CR_PLLON;
    while ((RCC_CR & RCC_CR_PLLRDY) == 0u) {
    }

    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_SW_MASK) | RCC_CFGR_SW_PLL;
    while ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) {
    }

    current_sysclk_hz = profile->sysclk_hz;
}

static uint32_t pclk1_hz_for_profile(const ClockProfile *profile)
{
    if (profile->apb1_div2) {
        return profile->sysclk_hz / 2u;
    }

    return profile->sysclk_hz;
}

static void show_running_for_profile_delay(void)
{
    uint8_t visible = 1u;

    for (uint32_t i = 0u; i < 6u; ++i) {
        OLED_DisplayRunning(visible);
        visible = (uint8_t)!visible;
        systick_delay_ms(500u);
    }
}

int main(void)
{
    uint32_t index = 0u;

    OLED_DisplayInit();

    while (1) {
        const ClockProfile *profile = &clock_profiles[index];
        uint32_t pclk1_hz;

        clock_apply_profile(profile);
        pclk1_hz = pclk1_hz_for_profile(profile);

        OLED_DisplayClockStatus(profile->sysclk_hz / 1000000u,
                                profile->sysclk_hz / 1000000u,
                                pclk1_hz / 1000000u,
                                profile->sysclk_hz / 1000000u);

        index = (index + 1u) %
                (sizeof(clock_profiles) / sizeof(clock_profiles[0]));
        show_running_for_profile_delay();
    }
}
