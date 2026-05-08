#include "oled_display.h"

#include "stm32f1_gpio.h"

#define OLED_ADDR 0x3Cu
#define OLED_WIDTH 128u
#define OLED_PAGES 8u

#define RCC_APB1ENR          (*(volatile uint32_t *)0x4002101Cu)
#define RCC_APB2ENR          (*(volatile uint32_t *)0x40021018u)
#define AFIO_MAPR            (*(volatile uint32_t *)0x40010004u)
#define GPIOB_CRH            (*(volatile uint32_t *)0x40010C04u)
#define GPIOB_IDR            (*(volatile uint32_t *)0x40010C08u)
#define GPIOB_BSRR           (*(volatile uint32_t *)0x40010C10u)
#define GPIOB_BRR            (*(volatile uint32_t *)0x40010C14u)

#define I2C1_CR1             (*(volatile uint32_t *)0x40005400u)
#define I2C1_CR2             (*(volatile uint32_t *)0x40005404u)
#define I2C1_DR              (*(volatile uint32_t *)0x40005410u)
#define I2C1_SR1             (*(volatile uint32_t *)0x40005414u)
#define I2C1_SR2             (*(volatile uint32_t *)0x40005418u)
#define I2C1_CCR             (*(volatile uint32_t *)0x4000541Cu)
#define I2C1_TRISE           (*(volatile uint32_t *)0x40005420u)

#define RCC_APB1ENR_I2C1EN   (1u << 21)
#define RCC_APB2ENR_IOPBEN   (1u << 3)
#define RCC_APB2ENR_AFIOEN   (1u << 0)
#define AFIO_MAPR_I2C1_REMAP (1u << 1)

#define I2C_CR1_PE           (1u << 0)
#define I2C_CR1_START        (1u << 8)
#define I2C_CR1_STOP         (1u << 9)
#define I2C_CR1_SWRST        (1u << 15)
#define I2C_SR1_SB           (1u << 0)
#define I2C_SR1_ADDR         (1u << 1)
#define I2C_SR1_BTF          (1u << 2)
#define I2C_SR1_TXE          (1u << 7)

#define I2C_TIMEOUT          100000u
#define OLED_SCL_PIN         8u
#define OLED_SDA_PIN         9u

static uint8_t oled_buffer[OLED_WIDTH * OLED_PAGES];
static uint8_t cursor_page;
static uint8_t cursor_column;
static uint8_t oled_use_software_i2c;

static const uint8_t font5x7[][5] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00},
    [','] = {0x00, 0x50, 0x30, 0x00, 0x00},
    ['-'] = {0x08, 0x08, 0x08, 0x08, 0x08},
    ['.'] = {0x00, 0x60, 0x60, 0x00, 0x00},
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
    ['A'] = {0x7E, 0x09, 0x09, 0x09, 0x7E},
    ['B'] = {0x7F, 0x49, 0x49, 0x49, 0x36},
    ['C'] = {0x3E, 0x41, 0x41, 0x41, 0x22},
    ['D'] = {0x7F, 0x41, 0x41, 0x22, 0x1C},
    ['E'] = {0x7F, 0x49, 0x49, 0x49, 0x41},
    ['F'] = {0x7F, 0x09, 0x09, 0x09, 0x01},
    ['G'] = {0x3E, 0x41, 0x49, 0x49, 0x7A},
    ['H'] = {0x7F, 0x08, 0x08, 0x08, 0x7F},
    ['I'] = {0x00, 0x41, 0x7F, 0x41, 0x00},
    ['J'] = {0x20, 0x40, 0x41, 0x3F, 0x01},
    ['K'] = {0x7F, 0x08, 0x14, 0x22, 0x41},
    ['L'] = {0x7F, 0x40, 0x40, 0x40, 0x40},
    ['M'] = {0x7F, 0x02, 0x0C, 0x02, 0x7F},
    ['N'] = {0x7F, 0x04, 0x08, 0x10, 0x7F},
    ['O'] = {0x3E, 0x41, 0x41, 0x41, 0x3E},
    ['P'] = {0x7F, 0x09, 0x09, 0x09, 0x06},
    ['Q'] = {0x3E, 0x41, 0x51, 0x21, 0x5E},
    ['R'] = {0x7F, 0x09, 0x19, 0x29, 0x46},
    ['S'] = {0x46, 0x49, 0x49, 0x49, 0x31},
    ['T'] = {0x01, 0x01, 0x7F, 0x01, 0x01},
    ['U'] = {0x3F, 0x40, 0x40, 0x40, 0x3F},
    ['V'] = {0x1F, 0x20, 0x40, 0x20, 0x1F},
    ['W'] = {0x7F, 0x20, 0x18, 0x20, 0x7F},
    ['X'] = {0x63, 0x14, 0x08, 0x14, 0x63},
    ['Y'] = {0x07, 0x08, 0x70, 0x08, 0x07},
    ['Z'] = {0x61, 0x51, 0x49, 0x45, 0x43},
    ['_'] = {0x40, 0x40, 0x40, 0x40, 0x40},
    ['e'] = {0x38, 0x54, 0x54, 0x54, 0x18},
    ['g'] = {0x08, 0x54, 0x54, 0x54, 0x3C},
    ['i'] = {0x00, 0x44, 0x7D, 0x40, 0x00},
    ['m'] = {0x7C, 0x04, 0x18, 0x04, 0x78},
    ['n'] = {0x7C, 0x08, 0x04, 0x04, 0x78},
    ['o'] = {0x38, 0x44, 0x44, 0x44, 0x38},
    ['t'] = {0x04, 0x3F, 0x44, 0x40, 0x20},
    ['u'] = {0x3C, 0x40, 0x40, 0x20, 0x7C},
    ['q'] = {0x08, 0x14, 0x14, 0x18, 0x7C},
    ['r'] = {0x7C, 0x08, 0x04, 0x04, 0x08},
    ['y'] = {0x0C, 0x50, 0x50, 0x50, 0x3C},
    ['z'] = {0x44, 0x64, 0x54, 0x4C, 0x44},
    ['%'] = {0x62, 0x64, 0x08, 0x13, 0x23},
};

static void delay(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm volatile ("nop");
    }
}

static void oled_bus_recover(void)
{
    GPIOB_CRH &= ~((0xFu << 0u) | (0xFu << 4u));
    GPIOB_CRH |=  ((0x6u << 0u) | (0x6u << 4u));
    GPIOB_BSRR = (1u << OLED_SCL_PIN) | (1u << OLED_SDA_PIN);

    for (uint8_t i = 0u; i < 9u; ++i) {
        GPIOB_BRR = 1u << OLED_SCL_PIN;
        delay(80u);
        GPIOB_BSRR = 1u << OLED_SCL_PIN;
        delay(80u);
    }

    GPIOB_BRR = 1u << OLED_SDA_PIN;
    delay(80u);
    GPIOB_BSRR = 1u << OLED_SCL_PIN;
    delay(80u);
    GPIOB_BSRR = 1u << OLED_SDA_PIN;
    delay(80u);

    (void)GPIOB_IDR;
}

static int i2c_wait_set(volatile uint32_t *reg, uint32_t flag)
{
    uint32_t timeout = I2C_TIMEOUT;

    while ((*reg & flag) == 0u) {
        if (timeout-- == 0u) {
            return 0;
        }
    }

    return 1;
}

static void soft_scl(uint8_t high)
{
    if (high) {
        GPIOB_BSRR = 1u << OLED_SCL_PIN;
    } else {
        GPIOB_BRR = 1u << OLED_SCL_PIN;
    }
}

static void soft_sda(uint8_t high)
{
    if (high) {
        GPIOB_BSRR = 1u << OLED_SDA_PIN;
    } else {
        GPIOB_BRR = 1u << OLED_SDA_PIN;
    }
}

static void soft_i2c_start(void)
{
    soft_sda(1u);
    soft_scl(1u);
    delay(30u);
    soft_sda(0u);
    delay(30u);
    soft_scl(0u);
}

static void soft_i2c_stop(void)
{
    soft_sda(0u);
    soft_scl(1u);
    delay(30u);
    soft_sda(1u);
    delay(30u);
}

static void soft_i2c_write(uint8_t value)
{
    for (uint8_t mask = 0x80u; mask != 0u; mask >>= 1u) {
        soft_sda((value & mask) != 0u);
        delay(30u);
        soft_scl(1u);
        delay(30u);
        soft_scl(0u);
    }

    soft_sda(1u);
    delay(30u);
    soft_scl(1u);
    delay(30u);
    soft_scl(0u);
}

static void soft_i2c_prepare(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPBEN;
    GPIOB_CRH &= ~((0xFu << 0u) | (0xFu << 4u));
    GPIOB_CRH |=  ((0x6u << 0u) | (0x6u << 4u));
    soft_scl(1u);
    soft_sda(1u);
}

static void oled_i2c1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;

    I2C1_CR1 = 0u;
    oled_bus_recover();

    AFIO_MAPR |= AFIO_MAPR_I2C1_REMAP;

    /*
     * PB8 I2C1_SCL, PB9 I2C1_SDA after remap:
     * alternate-function open-drain, 2 MHz.
     */
    GPIOB_CRH &= ~((0xFu << 0u) | (0xFu << 4u));
    GPIOB_CRH |=  ((0xEu << 0u) | (0xEu << 4u));

    I2C1_CR1 = I2C_CR1_SWRST;
    I2C1_CR1 = 0u;

    I2C1_CR2 = 8u;     /* APB1 is 8 MHz in the default projects. */
    I2C1_CCR = 40u;    /* Standard mode: 8 MHz / (2 * 100 kHz). */
    I2C1_TRISE = 9u;   /* 1000 ns / 125 ns + 1. */
    I2C1_CR1 = I2C_CR1_PE;
}

static int oled_i2c_begin(void)
{
    I2C1_CR1 |= I2C_CR1_START;
    if (!i2c_wait_set(&I2C1_SR1, I2C_SR1_SB)) {
        return 0;
    }

    I2C1_DR = OLED_ADDR << 1u;
    if (!i2c_wait_set(&I2C1_SR1, I2C_SR1_ADDR)) {
        I2C1_CR1 |= I2C_CR1_STOP;
        return 0;
    }

    (void)I2C1_SR1;
    (void)I2C1_SR2;
    return 1;
}

static int oled_i2c_write(uint8_t value)
{
    if (!i2c_wait_set(&I2C1_SR1, I2C_SR1_TXE)) {
        return 0;
    }

    I2C1_DR = value;
    return 1;
}

static void oled_i2c_end(void)
{
    (void)i2c_wait_set(&I2C1_SR1, I2C_SR1_BTF);
    I2C1_CR1 |= I2C_CR1_STOP;
}

static void oled_cmd(uint8_t command)
{
    if (oled_use_software_i2c) {
        soft_i2c_start();
        soft_i2c_write(OLED_ADDR << 1u);
        soft_i2c_write(0x00u);
        soft_i2c_write(command);
        soft_i2c_stop();
        return;
    }

    if (oled_i2c_begin()) {
        (void)oled_i2c_write(0x00u);
        (void)oled_i2c_write(command);
        oled_i2c_end();
    } else {
        oled_use_software_i2c = 1u;
        soft_i2c_prepare();
        oled_cmd(command);
    }
}

static void oled_set_pos(uint8_t page, uint8_t column)
{
    cursor_page = page;
    cursor_column = column;

    oled_cmd(0xB0u | page);
    oled_cmd(0x00u | (column & 0x0Fu));
    oled_cmd(0x10u | (column >> 4u));
}

static void oled_buffer_clear(void)
{
    for (uint32_t i = 0u; i < sizeof(oled_buffer); ++i) {
        oled_buffer[i] = 0u;
    }

    cursor_page = 0u;
    cursor_column = 0u;
}

static void oled_flush_page(uint8_t page)
{
    oled_set_pos(page, 0u);

    if (oled_use_software_i2c) {
        soft_i2c_start();
        soft_i2c_write(OLED_ADDR << 1u);
        soft_i2c_write(0x40u);
        for (uint8_t column = 0u; column < OLED_WIDTH; ++column) {
            soft_i2c_write(oled_buffer[(page * OLED_WIDTH) + column]);
        }
        soft_i2c_stop();
        return;
    }

    if (oled_i2c_begin()) {
        (void)oled_i2c_write(0x40u);
        for (uint8_t column = 0u; column < OLED_WIDTH; ++column) {
            (void)oled_i2c_write(oled_buffer[(page * OLED_WIDTH) + column]);
        }
        oled_i2c_end();
    } else {
        oled_use_software_i2c = 1u;
        soft_i2c_prepare();
        oled_flush_page(page);
    }
}

static void oled_flush_all(void)
{
    for (uint8_t page = 0u; page < OLED_PAGES; ++page) {
        oled_flush_page(page);
    }
}

static void oled_putc(char ch)
{
    const uint8_t *glyph = font5x7[(uint8_t)ch];

    if (cursor_page >= OLED_PAGES || cursor_column >= OLED_WIDTH) {
        return;
    }

    for (uint8_t i = 0u; i < 5u; ++i) {
        if ((cursor_column + i) < OLED_WIDTH) {
            oled_buffer[(cursor_page * OLED_WIDTH) + cursor_column + i] =
                glyph[i];
        }
    }
    if ((cursor_column + 5u) < OLED_WIDTH) {
        oled_buffer[(cursor_page * OLED_WIDTH) + cursor_column + 5u] = 0u;
    }

    cursor_column = (uint8_t)(cursor_column + 6u);
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
    uint8_t count = 0u;

    if (value == 0u) {
        oled_putc('0');
        return;
    }

    while (value > 0u && count < sizeof(digits)) {
        digits[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (count > 0u) {
        oled_putc(digits[--count]);
    }
}

static void oled_print_i32(int32_t value)
{
    if (value < 0) {
        oled_putc('-');
        value = -value;
    }

    oled_print_u32((uint32_t)value);
}

static void oled_print_hex_digit(uint8_t value)
{
    value &= 0x0Fu;
    if (value < 10u) {
        oled_putc((char)('0' + value));
    } else {
        oled_putc((char)('A' + value - 10u));
    }
}

static void oled_print_hex8(uint8_t value)
{
    oled_print_hex_digit(value >> 4u);
    oled_print_hex_digit(value);
}

static void oled_print_hex24(uint32_t value)
{
    oled_print_hex8((uint8_t)(value >> 16u));
    oled_print_hex8((uint8_t)(value >> 8u));
    oled_print_hex8((uint8_t)value);
}

static void oled_print_2digits(uint32_t value)
{
    oled_putc((char)('0' + ((value / 10u) % 10u)));
    oled_putc((char)('0' + (value % 10u)));
}

static void oled_print_4digits(uint32_t value)
{
    oled_putc((char)('0' + ((value / 1000u) % 10u)));
    oled_putc((char)('0' + ((value / 100u) % 10u)));
    oled_putc((char)('0' + ((value / 10u) % 10u)));
    oled_putc((char)('0' + (value % 10u)));
}

void OLED_DisplayInit(void)
{
    if (oled_use_software_i2c) {
        soft_i2c_prepare();
    } else {
        oled_i2c1_init();
    }

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
    oled_buffer_clear();
    oled_flush_all();
}

void OLED_DisplayUseSoftwareI2C(uint8_t enable)
{
    oled_use_software_i2c = enable ? 1u : 0u;
}

void OLED_DisplayFrequency(uint32_t frequency_hz)
{
    oled_set_pos(2u, 0u);
    oled_print("Freq:");
    oled_print("          ");

    oled_set_pos(4u, 0u);
    oled_print_u32(frequency_hz);
    oled_print(" Hz      ");
    oled_flush_all();
}

void OLED_DisplayMeasurement(uint32_t frequency_hz, uint32_t duty_percent)
{
    oled_set_pos(2u, 0u);
    oled_print("Freq:");
    oled_print_u32(frequency_hz);
    oled_print(" Hz   ");

    oled_set_pos(4u, 0u);
    oled_print("Duty:");
    oled_print_u32(duty_percent);
    oled_print(" %    ");
    oled_flush_all();
}

void OLED_DisplayAdcVoltage(uint32_t adc_value, uint32_t millivolts)
{
    oled_set_pos(2u, 0u);
    oled_print("AD:");
    oled_print_u32(adc_value);
    oled_print("    ");

    oled_set_pos(4u, 0u);
    oled_print("mV:");
    oled_print_u32(millivolts);
    oled_print("    ");
    oled_flush_all();
}

void OLED_DisplayTextStatus(const char *line1, const char *line2)
{
    oled_buffer_clear();
    oled_set_pos(2u, 0u);
    oled_print(line1);
    oled_print("                    ");

    oled_set_pos(4u, 0u);
    oled_print(line2);
    oled_print("                    ");
    oled_flush_all();
}

void OLED_DisplayMpu6050Raw(int16_t accel_x, int16_t accel_y, int16_t accel_z,
                            int16_t gyro_x, int16_t gyro_y, int16_t gyro_z,
                            int16_t temp_raw)
{
    oled_set_pos(0u, 0u);
    oled_print("MPU6050 RAW     ");

    oled_set_pos(1u, 0u);
    oled_print("AX:");
    oled_print_i32(accel_x);
    oled_print(" AY:");
    oled_print_i32(accel_y);
    oled_print("    ");

    oled_set_pos(2u, 0u);
    oled_print("AZ:");
    oled_print_i32(accel_z);
    oled_print("          ");

    oled_set_pos(4u, 0u);
    oled_print("GX:");
    oled_print_i32(gyro_x);
    oled_print(" GY:");
    oled_print_i32(gyro_y);
    oled_print("    ");

    oled_set_pos(5u, 0u);
    oled_print("GZ:");
    oled_print_i32(gyro_z);
    oled_print("          ");

    oled_set_pos(7u, 0u);
    oled_print("T:");
    oled_print_i32(temp_raw);
    oled_print("          ");
    oled_flush_all();
}

void OLED_DisplayW25q64Test(uint8_t manufacturer_id, uint8_t memory_type,
                            uint8_t capacity_id, uint32_t test_addr,
                            uint8_t pass)
{
    oled_buffer_clear();

    oled_set_pos(0u, 0u);
    oled_print("W25Q64 SOFT SPI");

    oled_set_pos(2u, 0u);
    oled_print("ID:");
    oled_print_hex8(manufacturer_id);
    oled_putc(' ');
    oled_print_hex8(memory_type);
    oled_putc(' ');
    oled_print_hex8(capacity_id);

    oled_set_pos(4u, 0u);
    oled_print("ADDR:");
    oled_print_hex24(test_addr);

    oled_set_pos(6u, 0u);
    if (pass) {
        oled_print("ERASE WRITE OK");
    } else {
        oled_print("TEST FAIL");
    }
    oled_flush_all();
}

void OLED_DisplayBkpTest(uint16_t previous, uint16_t current,
                         uint16_t signature, uint8_t pass)
{
    oled_buffer_clear();

    oled_set_pos(0u, 0u);
    oled_print("BKP REG TEST");

    oled_set_pos(2u, 0u);
    oled_print("PREV:");
    oled_print_hex8((uint8_t)(previous >> 8u));
    oled_print_hex8((uint8_t)previous);

    oled_set_pos(3u, 0u);
    oled_print("NOW :");
    oled_print_hex8((uint8_t)(current >> 8u));
    oled_print_hex8((uint8_t)current);

    oled_set_pos(5u, 0u);
    oled_print("SIG :");
    oled_print_hex8((uint8_t)(signature >> 8u));
    oled_print_hex8((uint8_t)signature);

    oled_set_pos(7u, 0u);
    if (pass) {
        oled_print("BKP WRITE OK");
    } else {
        oled_print("BKP FAIL");
    }
    oled_flush_all();
}

void OLED_DisplayRtcTime(uint32_t year, uint32_t month, uint32_t day,
                         uint32_t hours, uint32_t minutes, uint32_t seconds,
                         uint32_t counter, uint32_t elapsed_ms)
{
    static uint8_t layout_ready;

    if (!layout_ready) {
        oled_buffer_clear();

        oled_set_pos(0u, 0u);
        oled_print("RTC CLOCK");

        oled_set_pos(2u, 0u);
        oled_print("DATE:0000-00-00");

        oled_set_pos(4u, 0u);
        oled_print("TIME:00:00:00");

        oled_set_pos(6u, 0u);
        oled_print("CNT :0000000000");

        oled_set_pos(7u, 0u);
        oled_print("MS  :0000000000");

        layout_ready = 1u;
    }

    oled_set_pos(2u, 30u);
    oled_print_4digits(year);
    oled_putc('-');
    oled_print_2digits(month);
    oled_putc('-');
    oled_print_2digits(day);

    oled_set_pos(4u, 30u);
    oled_print_2digits(hours);
    oled_putc(':');
    oled_print_2digits(minutes);
    oled_putc(':');
    oled_print_2digits(seconds);

    oled_set_pos(6u, 30u);
    oled_print_u32(counter);
    oled_print("          ");

    oled_set_pos(7u, 30u);
    oled_print_u32(elapsed_ms);
    oled_print("          ");
    oled_flush_all();
}

void OLED_DisplayClockStatus(uint32_t sysclk_mhz, uint32_t hclk_mhz,
                             uint32_t pclk1_mhz, uint32_t pclk2_mhz)
{
    oled_buffer_clear();

    oled_set_pos(0u, 0u);
    oled_print("CLOCK CONTROL");

    oled_set_pos(2u, 0u);
    oled_print("SYS:");
    oled_print_u32(sysclk_mhz);
    oled_print(" MHz");

    oled_set_pos(3u, 0u);
    oled_print("HCLK:");
    oled_print_u32(hclk_mhz);
    oled_print(" MHz");

    oled_set_pos(5u, 0u);
    oled_print("P1:");
    oled_print_u32(pclk1_mhz);
    oled_print(" MHz");

    oled_set_pos(6u, 0u);
    oled_print("P2:");
    oled_print_u32(pclk2_mhz);
    oled_print(" MHz");
    oled_flush_all();
}

void OLED_DisplayRunning(uint8_t visible)
{
    oled_set_pos(7u, 0u);
    if (visible) {
        oled_print("Running");
    } else {
        oled_print("       ");
    }
    oled_flush_page(7u);
}

void OLED_DisplaySleepWake(uint32_t wake_count, uint8_t rx_byte)
{
    oled_buffer_clear();

    oled_set_pos(0u, 0u);
    oled_print("SLEEP USART");

    oled_set_pos(2u, 0u);
    oled_print("WAKE:");
    oled_print_u32(wake_count);

    oled_set_pos(4u, 0u);
    oled_print("RX HEX:");
    oled_print_hex8(rx_byte);

    oled_set_pos(6u, 0u);
    oled_print("WAIT NEXT RX");
    oled_flush_all();
}
