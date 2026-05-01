#include <stdint.h>

#include "stm32f1_gpio.h"

#define OLED_ADDR 0x3Cu
#define SCL_PIN   8u
#define SDA_PIN   9u

#define ENCODER_A_PIN 6u
#define ENCODER_B_PIN 7u

#define RCC_APB1ENR        (*(volatile uint32_t *)0x4002101Cu)
#define RCC_APB1ENR_TIM2EN (1u << 0)
#define RCC_APB1ENR_TIM3EN (1u << 1)

#define TIM2_CR1  (*(volatile uint32_t *)0x40000000u)
#define TIM2_DIER (*(volatile uint32_t *)0x4000000Cu)
#define TIM2_SR   (*(volatile uint32_t *)0x40000010u)
#define TIM2_EGR  (*(volatile uint32_t *)0x40000014u)
#define TIM2_PSC  (*(volatile uint32_t *)0x40000028u)
#define TIM2_ARR  (*(volatile uint32_t *)0x4000002Cu)

#define TIM3_CR1   (*(volatile uint32_t *)0x40000400u)
#define TIM3_SMCR  (*(volatile uint32_t *)0x40000408u)
#define TIM3_CNT   (*(volatile uint32_t *)0x40000424u)
#define TIM3_PSC   (*(volatile uint32_t *)0x40000428u)
#define TIM3_ARR   (*(volatile uint32_t *)0x4000042Cu)
#define TIM3_CCMR1 (*(volatile uint32_t *)0x40000418u)
#define TIM3_CCER  (*(volatile uint32_t *)0x40000420u)
#define TIM3_EGR   (*(volatile uint32_t *)0x40000414u)
#define TIM3_SR    (*(volatile uint32_t *)0x40000410u)

#define NVIC_ISER0 (*(volatile uint32_t *)0xE000E100u)
#define TIM2_IRQn  28u

#define TIM_CR1_CEN        (1u << 0)
#define TIM_DIER_UIE       (1u << 0)
#define TIM_SR_UIF         (1u << 0)
#define TIM_EGR_UG         (1u << 0)
#define TIM_SMCR_SMS_ENC3  (3u << 0)
#define TIM_CCMR1_CC1S_TI1 (1u << 0)
#define TIM_CCMR1_CC2S_TI2 (1u << 8)
#define TIM_CCMR1_IC1F_8   (3u << 4)
#define TIM_CCMR1_IC2F_8   (3u << 12)
#define TIM_CCER_CC1E      (1u << 0)
#define TIM_CCER_CC2E      (1u << 4)

#define SPEED_SAMPLE_HZ 10
#define ENCODER_COUNTS_PER_DETENT 4

static volatile int32_t encoder_count;
static volatile int32_t encoder_speed_cps;
static volatile int8_t encoder_direction;
static volatile uint8_t encoder_sample_ready;
static volatile uint16_t encoder_last_raw;

static const uint8_t font5x7[][5] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['+'] = {0x08, 0x08, 0x3E, 0x08, 0x08},
    ['-'] = {0x08, 0x08, 0x08, 0x08, 0x08},
    ['/'] = {0x20, 0x10, 0x08, 0x04, 0x02},
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
    ['D'] = {0x7F, 0x41, 0x41, 0x22, 0x1C},
    ['E'] = {0x7F, 0x49, 0x49, 0x49, 0x41},
    ['S'] = {0x26, 0x49, 0x49, 0x49, 0x32},
    ['c'] = {0x38, 0x44, 0x44, 0x44, 0x20},
    ['d'] = {0x38, 0x44, 0x44, 0x48, 0x7F},
    ['e'] = {0x38, 0x54, 0x54, 0x54, 0x18},
    ['i'] = {0x00, 0x44, 0x7D, 0x40, 0x00},
    ['n'] = {0x7C, 0x08, 0x04, 0x04, 0x78},
    ['o'] = {0x38, 0x44, 0x44, 0x44, 0x38},
    ['p'] = {0x7C, 0x14, 0x14, 0x14, 0x08},
    ['r'] = {0x7C, 0x08, 0x04, 0x04, 0x08},
    ['s'] = {0x48, 0x54, 0x54, 0x54, 0x20},
    ['t'] = {0x04, 0x3F, 0x44, 0x40, 0x20},
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

static void oled_print_i32(int32_t value)
{
    char digits[11];
    uint8_t length = 0;
    uint32_t magnitude;

    if (value < 0) {
        oled_putc('-');
        magnitude = (uint32_t)(-(value + 1)) + 1u;
    } else {
        magnitude = (uint32_t)value;
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

static void oled_print_direction(int8_t direction)
{
    if (direction > 0) {
        oled_putc('+');
    } else if (direction < 0) {
        oled_putc('-');
    } else {
        oled_putc('0');
    }
}

static void oled_show_encoder(int32_t count, int8_t direction, int32_t speed_cps)
{
    oled_clear_page(1u);
    oled_set_pos(1u, 0u);
    oled_print("Cnt:");
    oled_print_i32(count);

    oled_clear_page(3u);
    oled_set_pos(3u, 0u);
    oled_print("Dir:");
    oled_print_direction(direction);

    oled_clear_page(5u);
    oled_set_pos(5u, 0u);
    oled_print("Spd:");
    oled_print_i32(speed_cps);
    oled_print(" c/s");
}

static int32_t encoder_counts_to_detents(int32_t counts)
{
    return counts / ENCODER_COUNTS_PER_DETENT;
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

static void encoder_tim3_init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM3EN;

    GPIO_ConfigInputPullUp(GPIOA, ENCODER_A_PIN);
    GPIO_ConfigInputPullUp(GPIOA, ENCODER_B_PIN);

    TIM3_CR1 = 0u;
    TIM3_PSC = 0u;
    TIM3_ARR = 0xFFFFu;
    TIM3_CNT = 0u;
    TIM3_CCMR1 = TIM_CCMR1_CC1S_TI1 | TIM_CCMR1_CC2S_TI2 |
                 TIM_CCMR1_IC1F_8 | TIM_CCMR1_IC2F_8;
    TIM3_CCER = TIM_CCER_CC1E | TIM_CCER_CC2E;
    TIM3_SMCR = TIM_SMCR_SMS_ENC3;
    TIM3_EGR = TIM_EGR_UG;
    TIM3_SR = 0u;
    TIM3_CR1 = TIM_CR1_CEN;
}

static void timebase_tim2_init_100ms(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    TIM2_CR1 = 0u;
    TIM2_DIER = 0u;
    TIM2_PSC = 7999u; /* 8 MHz / 8000 = 1 kHz. */
    TIM2_ARR = 99u;   /* 1 kHz / 100 = 10 Hz. */
    TIM2_EGR = TIM_EGR_UG;
    TIM2_SR = 0u;
    TIM2_DIER = TIM_DIER_UIE;
    NVIC_ISER0 = 1u << TIM2_IRQn;
    TIM2_CR1 = TIM_CR1_CEN;
}

void TIM2_IRQHandler(void)
{
    if ((TIM2_SR & TIM_SR_UIF) != 0u) {
        uint16_t raw = (uint16_t)TIM3_CNT;
        int16_t delta = (int16_t)(raw - encoder_last_raw);

        TIM2_SR &= ~TIM_SR_UIF;
        encoder_last_raw = raw;
        encoder_count += delta;
        encoder_speed_cps = (int32_t)delta * SPEED_SAMPLE_HZ;

        if (delta > 0) {
            encoder_direction = 1;
        } else if (delta < 0) {
            encoder_direction = -1;
        } else {
            encoder_direction = 0;
        }

        encoder_sample_ready = 1u;
    }
}

int main(void)
{
    RCC_EnableGPIOA();
    RCC_EnableGPIOB();

    GPIO_ConfigOutputOpenDrain(GPIOB, SCL_PIN);
    GPIO_ConfigOutputOpenDrain(GPIOB, SDA_PIN);
    scl(GPIO_PIN_SET);
    sda(GPIO_PIN_SET);

    encoder_count = 0;
    encoder_speed_cps = 0;
    encoder_direction = 0;
    encoder_sample_ready = 0u;

    oled_init();
    oled_clear();
    encoder_tim3_init();
    encoder_last_raw = (uint16_t)TIM3_CNT;
    oled_show_encoder(0, 0, 0);
    timebase_tim2_init_100ms();

    while (1) {
        if (encoder_sample_ready != 0u) {
            int32_t count;
            int32_t speed_cps;
            int8_t direction;

            encoder_sample_ready = 0u;
            count = encoder_count;
            speed_cps = encoder_speed_cps;
            direction = encoder_direction;
            oled_show_encoder(encoder_counts_to_detents(count),
                              direction,
                              encoder_counts_to_detents(speed_cps));
        }
    }
}
