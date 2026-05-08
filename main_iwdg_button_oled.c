#include <stdint.h>

#include "oled_display.h"
#include "stm32f1_gpio.h"

#define RCC_APB1ENR          (*(volatile uint32_t *)0x4002101Cu)
#define RCC_APB2ENR          (*(volatile uint32_t *)0x40021018u)
#define RCC_CSR              (*(volatile uint32_t *)0x40021024u)
#define PWR_CR               (*(volatile uint32_t *)0x40007000u)

#define IWDG_KR              (*(volatile uint32_t *)0x40003000u)
#define IWDG_PR              (*(volatile uint32_t *)0x40003004u)
#define IWDG_RLR             (*(volatile uint32_t *)0x40003008u)
#define IWDG_SR              (*(volatile uint32_t *)0x4000300Cu)

#define SYSTICK_CSR          (*(volatile uint32_t *)0xE000E010u)
#define SYSTICK_RVR          (*(volatile uint32_t *)0xE000E014u)
#define SYSTICK_CVR          (*(volatile uint32_t *)0xE000E018u)

#define RCC_APB1ENR_BKPEN    (1u << 27)
#define RCC_APB1ENR_PWREN    (1u << 28)
#define RCC_APB2ENR_IOPBEN   (1u << 3)
#define RCC_CSR_LSION        (1u << 0)
#define RCC_CSR_LSIRDY       (1u << 1)
#define RCC_CSR_RMVF         (1u << 24)
#define RCC_CSR_IWDGRSTF     (1u << 29)
#define PWR_CR_DBP           (1u << 8)

#define BKP_DR1              (*(volatile uint32_t *)0x40006C04u)
#define BKP_DR2              (*(volatile uint32_t *)0x40006C08u)
#define BKP_SIGNATURE        0x1D06u

#define IWDG_KR_RELOAD       0xAAAAu
#define IWDG_KR_ENABLE       0xCCCCu
#define IWDG_KR_WRITE_ACCESS 0x5555u
#define IWDG_PR_DIV64        3u
#define IWDG_RELOAD_5S       3125u
#define IWDG_SR_BUSY_MASK    0x3u

#define KEY1_PIN             1u

static uint8_t iwdg_init_error;

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

static void key_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPBEN;
    GPIO_ConfigInputPullUp(GPIOB, KEY1_PIN);
}

static uint8_t key1_pressed(void)
{
    return GPIO_ReadPin(GPIOB, KEY1_PIN) == GPIO_PIN_RESET ? 1u : 0u;
}

static uint8_t key1_wait_release(void)
{
    uint16_t hold_ms = 0u;

    if (!key1_pressed()) {
        return 0u;
    }

    delay_ms(20u);
    if (!key1_pressed()) {
        return 0u;
    }

    while (key1_pressed()) {
        delay_ms(20u);
        hold_ms = (uint16_t)(hold_ms + 20u);
    }

    delay_ms(20u);
    return hold_ms >= 800u ? 2u : 1u;
}

static void backup_domain_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
    PWR_CR |= PWR_CR_DBP;
}

static uint8_t iwdg_reset_flag_is_set(void)
{
    return (RCC_CSR & RCC_CSR_IWDGRSTF) != 0u ? 1u : 0u;
}

static uint16_t reset_count_load(uint8_t iwdg_reset)
{
    uint16_t count;

    if ((BKP_DR2 & 0xFFFFu) != BKP_SIGNATURE) {
        BKP_DR2 = BKP_SIGNATURE;
        BKP_DR1 = 0u;
    }

    count = (uint16_t)(BKP_DR1 & 0xFFFFu);
    if (iwdg_reset) {
        count = (uint16_t)(count + 1u);
        BKP_DR1 = count;
    }

    RCC_CSR |= RCC_CSR_RMVF;
    return count;
}

static void make_value_text(char *text, const char *prefix, uint16_t value)
{
    text[0] = prefix[0];
    text[1] = prefix[1];
    text[2] = prefix[2];
    text[3] = prefix[3];
    text[4] = ':';
    text[5] = (char)('0' + ((value / 10000u) % 10u));
    text[6] = (char)('0' + ((value / 1000u) % 10u));
    text[7] = (char)('0' + ((value / 100u) % 10u));
    text[8] = (char)('0' + ((value / 10u) % 10u));
    text[9] = (char)('0' + (value % 10u));
    text[10] = '\0';
}

static uint8_t iwdg_init(void)
{
    uint32_t timeout = 2000000u;

    iwdg_init_error = 0u;

    RCC_CSR |= RCC_CSR_LSION;
    while (((RCC_CSR & RCC_CSR_LSIRDY) == 0u) && (timeout-- > 0u)) {
    }

    if ((RCC_CSR & RCC_CSR_LSIRDY) == 0u) {
        iwdg_init_error = 1u;
        return 0u;
    }

    IWDG_KR = IWDG_KR_WRITE_ACCESS;
    IWDG_PR = IWDG_PR_DIV64;
    IWDG_RLR = IWDG_RELOAD_5S;
    IWDG_KR = IWDG_KR_RELOAD;
    IWDG_KR = IWDG_KR_ENABLE;

    timeout = 8000000u;
    while (((IWDG_SR & IWDG_SR_BUSY_MASK) != 0u) && (timeout-- > 0u)) {
        IWDG_KR = IWDG_KR_RELOAD;
    }

    if ((IWDG_SR & IWDG_SR_BUSY_MASK) != 0u) {
        iwdg_init_error = 2u;
        return 0u;
    }

    IWDG_KR = IWDG_KR_RELOAD;
    return 1u;
}

static void iwdg_feed(void)
{
    IWDG_KR = IWDG_KR_RELOAD;
}

int main(void)
{
    uint16_t reset_count;
    uint16_t feed_count = 0u;
    uint8_t iwdg_reset;
    uint8_t key_event;
    char text[11];

    OLED_DisplayUseSoftwareI2C(1u);
    OLED_DisplayInit();
    key_init();
    backup_domain_init();

    OLED_DisplayTextStatus("IWDG TEST1", "INIT");
    iwdg_reset = iwdg_reset_flag_is_set();
    reset_count = reset_count_load(iwdg_reset);

    if (iwdg_reset) {
        OLED_DisplayTextStatus("IWDG RESET", "LAST BOOT");
    } else {
        OLED_DisplayTextStatus("NORMAL RST", "LAST BOOT");
    }
    delay_ms(600u);

    make_value_text(text, "RST ", reset_count);
    OLED_DisplayTextStatus("IWDG TEST2", text);
    delay_ms(600u);

    if (!iwdg_init()) {
        if (iwdg_init_error == 1u) {
            OLED_DisplayTextStatus("IWDG INIT", "LSI FAIL");
        } else if (iwdg_init_error == 2u) {
            OLED_DisplayTextStatus("IWDG INIT", "SR FAIL");
        } else {
            OLED_DisplayTextStatus("IWDG INIT", "FAIL");
        }
        while (1) {
        }
    }

    while (1) {

        key_event = key1_wait_release();

        if (key_event == 1u) {
            make_value_text(text, "KEY1", feed_count);
            OLED_DisplayTextStatus("KEY1 PB1", text);
            delay_ms(200u);
        }

        iwdg_feed();
        feed_count++;
        make_value_text(text, "FEED", feed_count);
        OLED_DisplayTextStatus("IWDG FEED", text);
        delay_ms(200u);
        OLED_DisplayTextStatus("IWDG TEST3", "HOLD PB1 RST");
        delay_ms(500u);
    }
}
