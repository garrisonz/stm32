#include <stdint.h>

#include "stm32f1_gpio.h"

#define OLED_ADDR 0x3Cu
#define SCL_PIN   8u
#define SDA_PIN   9u

#define ENCODER_A_PIN 6u
#define ENCODER_B_PIN 7u
#define ENCODER_A_LINE (1u << ENCODER_A_PIN)
#define ENCODER_B_LINE (1u << ENCODER_B_PIN)

#define RCC_APB2ENR       (*(volatile uint32_t *)0x40021018u)
#define RCC_APB2ENR_AFIOEN (1u << 0)

#define EXTI_IMR  (*(volatile uint32_t *)0x40010400u)
#define EXTI_RTSR (*(volatile uint32_t *)0x40010408u)
#define EXTI_FTSR (*(volatile uint32_t *)0x4001040Cu)
#define EXTI_PR   (*(volatile uint32_t *)0x40010414u)

#define NVIC_ISER0       (*(volatile uint32_t *)0xE000E100u)
#define EXTI9_5_IRQn     23u

static volatile int16_t encoder_delta;

static const uint8_t font5x7[][5] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['-'] = {0x08, 0x08, 0x08, 0x08, 0x08},
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
    ['E'] = {0x7F, 0x49, 0x49, 0x49, 0x41},
    ['N'] = {0x7F, 0x02, 0x0C, 0x10, 0x7F},
    ['T'] = {0x01, 0x01, 0x7F, 0x01, 0x01},
    ['X'] = {0x63, 0x14, 0x08, 0x14, 0x63},
    ['I'] = {0x00, 0x41, 0x7F, 0x41, 0x00},
    ['u'] = {0x3C, 0x40, 0x40, 0x20, 0x7C},
    ['m'] = {0x7C, 0x04, 0x18, 0x04, 0x78},
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
    oled_set_pos(page, 0u);
    for (uint8_t column = 0u; column < 128u; ++column) {
        oled_data(0x00u);
    }
}

static void oled_clear(void)
{
    for (uint8_t page = 0u; page < 8u; ++page) {
        oled_clear_page(page);
    }
}

static void oled_putc(char ch)
{
    const uint8_t *glyph = font5x7[(uint8_t)ch];

    for (uint8_t i = 0u; i < 5u; ++i) {
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

static void oled_print_i16(int16_t value)
{
    char digits[6];
    uint8_t length = 0u;
    uint16_t magnitude;

    if (value < 0) {
        oled_putc('-');
        magnitude = (uint16_t)(-(value + 1)) + 1u;
    } else {
        magnitude = (uint16_t)value;
    }

    if (magnitude == 0u) {
        oled_putc('0');
        return;
    }

    while (magnitude > 0u && length < sizeof(digits)) {
        digits[length++] = (char)('0' + (magnitude % 10u));
        magnitude /= 10u;
    }

    while (length > 0u) {
        oled_putc(digits[--length]);
    }
}

static void oled_show_count(int16_t count)
{
    oled_clear_page(1u);
    oled_set_pos(1u, 0u);
    oled_print("EXTI");

    oled_clear_page(3u);
    oled_set_pos(3u, 0u);
    oled_print("Num:");
    oled_print_i16(count);
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

static void encoder_exti_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN;

    GPIO_ConfigInputPullUp(GPIOA, ENCODER_A_PIN);
    GPIO_ConfigInputPullUp(GPIOA, ENCODER_B_PIN);

    EXTI_IMR |= ENCODER_A_LINE | ENCODER_B_LINE;
    EXTI_RTSR &= ~(ENCODER_A_LINE | ENCODER_B_LINE);
    EXTI_FTSR |= ENCODER_A_LINE | ENCODER_B_LINE;
    EXTI_PR = ENCODER_A_LINE | ENCODER_B_LINE;

    NVIC_ISER0 = 1u << EXTI9_5_IRQn;
}

int16_t encoder_get(void)
{
    int16_t value = encoder_delta;

    encoder_delta = 0;
    return value;
}

void EXTI9_5_IRQHandler(void)
{
    uint32_t pending = EXTI_PR & (ENCODER_A_LINE | ENCODER_B_LINE);

    if ((pending & ENCODER_A_LINE) != 0u) {
        if (GPIO_ReadPin(GPIOA, ENCODER_A_PIN) == GPIO_PIN_RESET &&
            GPIO_ReadPin(GPIOA, ENCODER_B_PIN) == GPIO_PIN_RESET) {
            --encoder_delta;
        }
        EXTI_PR = ENCODER_A_LINE;
    }

    if ((pending & ENCODER_B_LINE) != 0u) {
        if (GPIO_ReadPin(GPIOA, ENCODER_B_PIN) == GPIO_PIN_RESET &&
            GPIO_ReadPin(GPIOA, ENCODER_A_PIN) == GPIO_PIN_RESET) {
            ++encoder_delta;
        }
        EXTI_PR = ENCODER_B_LINE;
    }
}

int main(void)
{
    int16_t count = 0;

    RCC_EnableGPIOA();
    RCC_EnableGPIOB();

    GPIO_ConfigOutputOpenDrain(GPIOB, SCL_PIN);
    GPIO_ConfigOutputOpenDrain(GPIOB, SDA_PIN);
    scl(GPIO_PIN_SET);
    sda(GPIO_PIN_SET);

    oled_init();
    oled_clear();
    encoder_exti_init();
    oled_show_count(count);

    while (1) {
        int16_t delta = encoder_get();

        if (delta != 0) {
            count += delta;
            oled_show_count(count);
        }
    }
}
