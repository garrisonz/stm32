#include <stdint.h>

#include "stm32f1_gpio.h"

#define RCC_APB2ENR  (*(volatile uint32_t *)0x40021018u)
#define GPIOA_CRH    (*(volatile uint32_t *)0x40010804u)
#define USART1_SR    (*(volatile uint32_t *)0x40013800u)
#define USART1_DR    (*(volatile uint32_t *)0x40013804u)
#define USART1_BRR   (*(volatile uint32_t *)0x40013808u)
#define USART1_CR1   (*(volatile uint32_t *)0x4001380Cu)

#define RCC_APB2ENR_IOPAEN   (1u << 2)
#define RCC_APB2ENR_USART1EN (1u << 14)

#define USART_SR_TXE         (1u << 7)
#define USART_SR_TC          (1u << 6)
#define USART_CR1_RE         (1u << 2)
#define USART_CR1_TE         (1u << 3)
#define USART_CR1_UE         (1u << 13)

#define SYSTICK_CSR          (*(volatile uint32_t *)0xE000E010u)
#define SYSTICK_RVR          (*(volatile uint32_t *)0xE000E014u)
#define SYSTICK_CVR          (*(volatile uint32_t *)0xE000E018u)

#define I2C_PORT             GPIOB
#define I2C_SCL_PIN          10u
#define I2C_SDA_PIN          11u

#define MPU6050_ADDR         0x68u
#define MPU6050_WHO_AM_I     0x75u
#define MPU6050_PWR_MGMT_1   0x6Bu
#define MPU6050_ACCEL_XOUT_H 0x3Bu

static void delay_cycles(volatile uint32_t cycles)
{
    while (cycles-- > 0u) {
    }
}

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

static void usart1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    GPIOA_CRH &= ~((0xFu << 4u) | (0xFu << 8u));
    GPIOA_CRH |=  ((0xAu << 4u) | (0x4u << 8u));

    USART1_CR1 = 0u;
    USART1_BRR = 0x0341u; /* 8 MHz PCLK2, 9600 baud. */
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void usart1_write_char(char ch)
{
    while ((USART1_SR & USART_SR_TXE) == 0u) {
    }

    USART1_DR = (uint32_t)(uint8_t)ch;
}

static void usart1_write_string(const char *text)
{
    while (*text != '\0') {
        usart1_write_char(*text++);
    }

    while ((USART1_SR & USART_SR_TC) == 0u) {
    }
}

static void usart1_write_int32(int32_t value)
{
    char digits[11];
    uint32_t index = 0u;
    uint32_t number;

    if (value < 0) {
        usart1_write_char('-');
        number = (uint32_t)(-value);
    } else {
        number = (uint32_t)value;
    }

    if (number == 0u) {
        usart1_write_char('0');
        return;
    }

    while (number > 0u) {
        digits[index++] = (char)('0' + (number % 10u));
        number /= 10u;
    }

    while (index > 0u) {
        usart1_write_char(digits[--index]);
    }
}

static void i2c_delay(void)
{
    delay_cycles(20u);
}

static void i2c_scl_high(void)
{
    GPIO_WritePin(I2C_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
}

static void i2c_scl_low(void)
{
    GPIO_WritePin(I2C_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
}

static void i2c_sda_high(void)
{
    GPIO_WritePin(I2C_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
}

static void i2c_sda_low(void)
{
    GPIO_WritePin(I2C_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
}

static GPIO_PinState i2c_sda_read(void)
{
    return GPIO_ReadPin(I2C_PORT, I2C_SDA_PIN);
}

static void soft_i2c_init(void)
{
    RCC_EnableGPIOB();
    GPIO_ConfigOutputOpenDrain(I2C_PORT, I2C_SCL_PIN);
    GPIO_ConfigOutputOpenDrain(I2C_PORT, I2C_SDA_PIN);
    i2c_scl_high();
    i2c_sda_high();
}

static void i2c_start(void)
{
    i2c_sda_high();
    i2c_scl_high();
    i2c_delay();
    i2c_sda_low();
    i2c_delay();
    i2c_scl_low();
}

static void i2c_stop(void)
{
    i2c_sda_low();
    i2c_delay();
    i2c_scl_high();
    i2c_delay();
    i2c_sda_high();
    i2c_delay();
}

static int i2c_write_byte(uint8_t value)
{
    for (uint32_t mask = 0x80u; mask > 0u; mask >>= 1) {
        if ((value & mask) != 0u) {
            i2c_sda_high();
        } else {
            i2c_sda_low();
        }

        i2c_delay();
        i2c_scl_high();
        i2c_delay();
        i2c_scl_low();
    }

    i2c_sda_high();
    i2c_delay();
    i2c_scl_high();
    i2c_delay();
    int ack = i2c_sda_read() == GPIO_PIN_RESET;
    i2c_scl_low();

    return ack;
}

static uint8_t i2c_read_byte(int ack)
{
    uint8_t value = 0u;

    i2c_sda_high();
    for (uint32_t bit = 0u; bit < 8u; ++bit) {
        value <<= 1;
        i2c_scl_high();
        i2c_delay();
        if (i2c_sda_read() == GPIO_PIN_SET) {
            value |= 1u;
        }
        i2c_scl_low();
        i2c_delay();
    }

    if (ack) {
        i2c_sda_low();
    } else {
        i2c_sda_high();
    }
    i2c_delay();
    i2c_scl_high();
    i2c_delay();
    i2c_scl_low();
    i2c_sda_high();

    return value;
}

static int mpu6050_write_reg(uint8_t reg, uint8_t value)
{
    i2c_start();
    if (!i2c_write_byte((MPU6050_ADDR << 1) | 0u)) {
        i2c_stop();
        return 0;
    }
    if (!i2c_write_byte(reg)) {
        i2c_stop();
        return 0;
    }
    if (!i2c_write_byte(value)) {
        i2c_stop();
        return 0;
    }
    i2c_stop();

    return 1;
}

static int mpu6050_read_regs(uint8_t reg, uint8_t *data, uint32_t length)
{
    i2c_start();
    if (!i2c_write_byte((MPU6050_ADDR << 1) | 0u)) {
        i2c_stop();
        return 0;
    }
    if (!i2c_write_byte(reg)) {
        i2c_stop();
        return 0;
    }

    i2c_start();
    if (!i2c_write_byte((MPU6050_ADDR << 1) | 1u)) {
        i2c_stop();
        return 0;
    }

    for (uint32_t i = 0u; i < length; ++i) {
        data[i] = i2c_read_byte(i + 1u < length);
    }
    i2c_stop();

    return 1;
}

static int mpu6050_read_reg(uint8_t reg, uint8_t *value)
{
    return mpu6050_read_regs(reg, value, 1u);
}

static int16_t read_i16_be(const uint8_t *data)
{
    return (int16_t)((uint16_t)data[0] << 8 | data[1]);
}

static void print_sample(const uint8_t *raw)
{
    int16_t accel_x = read_i16_be(&raw[0]);
    int16_t accel_y = read_i16_be(&raw[2]);
    int16_t accel_z = read_i16_be(&raw[4]);
    int16_t temp = read_i16_be(&raw[6]);
    int16_t gyro_x = read_i16_be(&raw[8]);
    int16_t gyro_y = read_i16_be(&raw[10]);
    int16_t gyro_z = read_i16_be(&raw[12]);

    usart1_write_string("accel=");
    usart1_write_int32(accel_x);
    usart1_write_char(',');
    usart1_write_int32(accel_y);
    usart1_write_char(',');
    usart1_write_int32(accel_z);
    usart1_write_string(" temp_raw=");
    usart1_write_int32(temp);
    usart1_write_string(" gyro=");
    usart1_write_int32(gyro_x);
    usart1_write_char(',');
    usart1_write_int32(gyro_y);
    usart1_write_char(',');
    usart1_write_int32(gyro_z);
    usart1_write_string("\r\n");
}

int main(void)
{
    uint8_t who_am_i = 0u;
    uint8_t raw[14];

    usart1_init();
    soft_i2c_init();

    usart1_write_string("MPU-6050 soft I2C on PB10/PB11\r\n");
    delay_ms(100u);

    while (!mpu6050_read_reg(MPU6050_WHO_AM_I, &who_am_i)) {
        usart1_write_string("MPU-6050 not responding\r\n");
        delay_ms(1000u);
    }

    usart1_write_string("WHO_AM_I=");
    usart1_write_int32(who_am_i);
    usart1_write_string("\r\n");

    if (!mpu6050_write_reg(MPU6050_PWR_MGMT_1, 0x00u)) {
        usart1_write_string("Failed to wake MPU-6050\r\n");
    }
    delay_ms(100u);

    while (1) {
        if (mpu6050_read_regs(MPU6050_ACCEL_XOUT_H, raw, sizeof(raw))) {
            print_sample(raw);
        } else {
            usart1_write_string("MPU-6050 read failed\r\n");
        }
        delay_ms(500u);
    }
}
