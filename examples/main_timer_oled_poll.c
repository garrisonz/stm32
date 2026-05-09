#include <stdint.h>

#include "stm32f1_gpio.h"

#define OLED_ADDR 0x3Cu
#define SCL_PIN   8u
#define SDA_PIN   9u

#define RCC_APB1ENR       (*(volatile uint32_t *)0x4002101Cu)
#define RCC_APB1ENR_TIM2EN (1u << 0)

#define TIM2_CR1  (*(volatile uint32_t *)0x40000000u)
#define TIM2_SR   (*(volatile uint32_t *)0x40000010u)
#define TIM2_EGR  (*(volatile uint32_t *)0x40000014u)
#define TIM2_PSC  (*(volatile uint32_t *)0x40000028u)
#define TIM2_ARR  (*(volatile uint32_t *)0x4000002Cu)

#define TIM_CR1_CEN (1u << 0)
#define TIM_SR_UIF  (1u << 0)
#define TIM_EGR_UG  (1u << 0)

static const uint8_t font5x7[][5] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00},
    [':'] = {0x00, 0x36, 0x36, 0x00, 0x00},
    ['0'] = {0x3E, 0x51, 0x49, 0x45, 0x3E},
    ['1'] = {0x00, 0x42, 0x7F, 0x40, 0x00},
    ['2'] = {0x42, 0x61, 0x51, 0x49, 0x46},
    ['3'] = {0x21, 0x41, 0x45, 0x4B, 0x31},
    ['4'] = {0x18, 0x14, 0x12, 0x7F, 0x10},
    ['5'] = {0x27, 0x45, 0x45, 0x45, 0x39},
    ['6'] = {0x3C, 0x4A, 0x49, 0x49, 0x30},
    ['7'] = {0x01, 0x71, 0x09, 0x05, 0x03},
    ['8'] = {0x36, 0x49, 0x49, 0x49, 0x36},
    ['9'] = {0x06, 0x49, 0x49, 0x29, 0x1E},
    ['C'] = {0x3E, 0x41, 0x41, 0x41, 0x22},
    ['T'] = {0x01, 0x01, 0x7F, 0x01, 0x01},
    ['e'] = {0x38, 0x54, 0x54, 0x54, 0x18},
    ['i'] = {0x00, 0x44, 0x7D, 0x40, 0x00},
    ['m'] = {0x7C, 0x04, 0x18, 0x04, 0x78},
    ['n'] = {0x7C, 0x08, 0x04, 0x04, 0x78},
    ['o'] = {0x38, 0x44, 0x44, 0x44, 0x38},
    ['r'] = {0x7C, 0x08, 0x04, 0x04, 0x08},
    ['t'] = {0x04, 0x3F, 0x44, 0x40, 0x20},
    ['u'] = {0x3C, 0x40, 0x40, 0x20, 0x7C},
};

static void delay(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm volatile ("nop");
    }
}

static void scl(GPIO_PinState state)
{
    GPIO_WritePin(GPIOB, SCL_PIN, state);
}

static void sda(GPIO_PinState state)
{
    GPIO_WritePin(GPIOB, SDA_PIN, state);
}

static void i2c_delay(void)
{
    delay(30u);
}

static void i2c_start(void)
{
    sda(GPIO_PIN_SET);
    scl(GPIO_PIN_SET);
    i2c_delay();
    sda(GPIO_PIN_RESET);
    i2c_delay();
    scl(GPIO_PIN_RESET);
}

static void i2c_stop(void)
{
    sda(GPIO_PIN_RESET);
    scl(GPIO_PIN_SET);
    i2c_delay();
    sda(GPIO_PIN_SET);
    i2c_delay();
}

static void i2c_write_byte(uint8_t value)
{
    for (uint8_t mask = 0x80u; mask != 0u; mask >>= 1u) {
        sda((value & mask) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        i2c_delay();
        scl(GPIO_PIN_SET);
        i2c_delay();
        scl(GPIO_PIN_RESET);
    }

    sda(GPIO_PIN_SET);
    i2c_delay();
    scl(GPIO_PIN_SET);
    i2c_delay();
    scl(GPIO_PIN_RESET);
}

static void oled_write(uint8_t control, uint8_t value)
{
    i2c_start();
    i2c_write_byte((OLED_ADDR << 1u) | 0u);
    i2c_write_byte(control);
    i2c_write_byte(value);
    i2c_stop();
}

static void oled_cmd(uint8_t command)
{
    oled_write(0x00u, command);
}

static void oled_data(uint8_t data)
{
    oled_write(0x40u, data);
}

static void oled_set_pos(uint8_t page, uint8_t column)
{
    oled_cmd(0xB0u | page);
    oled_cmd(0x00u | (column & 0x0Fu));
    oled_cmd(0x10u | (column >> 4u));
}

static void oled_clear_page(uint8_t page)
{
    oled_set_pos(page, 0);
    for (uint8_t column = 0; column < 128u; ++column) {
        oled_data(0x00u);
    }
}

static void oled_clear(void)
{
    for (uint8_t page = 0; page < 8u; ++page) {
        oled_clear_page(page);
    }
}

static void oled_putc(char ch)
{
    const uint8_t *glyph = font5x7[(uint8_t)ch];

    for (uint8_t i = 0; i < 5u; ++i) {
        oled_data(glyph[i]);
    }
    oled_data(0x00u);
}

static void oled_print(const char *text)
{
    while (*text != '\0') {
        oled_putc(*text++);
    }
}

static void oled_print_u32(uint32_t value)
{
    char digits[10];
    uint8_t length = 0;

    if (value == 0u) {
        oled_putc('0');
        return;
    }

    while (value > 0u && length < sizeof(digits)) {
        digits[length++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (length > 0u) {
        oled_putc(digits[--length]);
    }
}

static void oled_show_count(uint32_t count)
{
    oled_clear_page(3u);
    oled_set_pos(3u, 28u);
    oled_print("Timer:");
    oled_clear_page(5u);
    oled_set_pos(5u, 50u);
    oled_print_u32(count);
}

static void oled_init(void)
{
    delay(800000u);
    oled_cmd(0xAEu);
    oled_cmd(0x20u);
    oled_cmd(0x02u);
    oled_cmd(0xB0u);
    oled_cmd(0xC8u);
    oled_cmd(0x00u);
    oled_cmd(0x10u);
    oled_cmd(0x40u);
    oled_cmd(0x81u);
    oled_cmd(0x7Fu);
    oled_cmd(0xA1u);
    oled_cmd(0xA6u);
    oled_cmd(0xA8u);
    oled_cmd(0x3Fu);
    oled_cmd(0xA4u);
    oled_cmd(0xD3u);
    oled_cmd(0x00u);
    oled_cmd(0xD5u);
    oled_cmd(0x80u);
    oled_cmd(0xD9u);
    oled_cmd(0xF1u);
    oled_cmd(0xDAu);
    oled_cmd(0x12u);
    oled_cmd(0xDBu);
    oled_cmd(0x40u);
    oled_cmd(0x8Du);
    oled_cmd(0x14u);
    oled_cmd(0xAFu);
}

static void tim2_init_1s(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2_CR1 = 0u;
    TIM2_PSC = 7999u; /* 8 MHz / (7999 + 1) = 1 kHz. */
    TIM2_ARR = 99u;  /* 1 kHz / (999 + 1) = 1 Hz. */
    TIM2_EGR = TIM_EGR_UG;
    TIM2_SR = 0u;
    TIM2_CR1 = TIM_CR1_CEN;
}

static uint8_t tim2_elapsed_1s(void)
{
    if ((TIM2_SR & TIM_SR_UIF) == 0u) {
        return 0u;
    }

    TIM2_SR &= ~TIM_SR_UIF;
    return 1u;
}

int main(void)
{
    uint32_t count = 0;

    RCC_EnableGPIOB();
    GPIO_ConfigOutputOpenDrain(GPIOB, SCL_PIN);
    GPIO_ConfigOutputOpenDrain(GPIOB, SDA_PIN);
    scl(GPIO_PIN_SET);
    sda(GPIO_PIN_SET);

    oled_init();
    oled_clear();
    oled_show_count(count);
    tim2_init_1s();

    while (1) {
        if (tim2_elapsed_1s()) {
            ++count;
            oled_show_count(count);
        }
    }
}
