#include <stdint.h>

#include "oled_display.h"
#include "stm32f1_gpio.h"

#define RCC_APB1ENR          (*(volatile uint32_t *)0x4002101Cu)
#define RCC_APB2ENR          (*(volatile uint32_t *)0x40021018u)
#define GPIOB_CRH            (*(volatile uint32_t *)0x40010C04u)

#define RCC_APB1ENR_I2C2EN   (1u << 22)
#define RCC_APB2ENR_IOPBEN   (1u << 3)
#define RCC_APB2ENR_AFIOEN   (1u << 0)

#define I2C2_CR1             (*(volatile uint32_t *)0x40005800u)
#define I2C2_CR2             (*(volatile uint32_t *)0x40005804u)
#define I2C2_DR              (*(volatile uint32_t *)0x40005810u)
#define I2C2_SR1             (*(volatile uint32_t *)0x40005814u)
#define I2C2_SR2             (*(volatile uint32_t *)0x40005818u)
#define I2C2_CCR             (*(volatile uint32_t *)0x4000581Cu)
#define I2C2_TRISE           (*(volatile uint32_t *)0x40005820u)

#define I2C_CR1_PE           (1u << 0)
#define I2C_CR1_START        (1u << 8)
#define I2C_CR1_STOP         (1u << 9)
#define I2C_CR1_ACK          (1u << 10)
#define I2C_CR1_SWRST        (1u << 15)
#define I2C_SR1_SB           (1u << 0)
#define I2C_SR1_ADDR         (1u << 1)
#define I2C_SR1_BTF          (1u << 2)
#define I2C_SR1_RXNE         (1u << 6)
#define I2C_SR1_TXE          (1u << 7)

#define SYSTICK_CSR          (*(volatile uint32_t *)0xE000E010u)
#define SYSTICK_RVR          (*(volatile uint32_t *)0xE000E014u)
#define SYSTICK_CVR          (*(volatile uint32_t *)0xE000E018u)

#define MPU6050_ADDR         0x68u
#define MPU6050_WHO_AM_I     0x75u
#define MPU6050_PWR_MGMT_1   0x6Bu
#define MPU6050_ACCEL_XOUT_H 0x3Bu

#define I2C_TIMEOUT          100000u

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

static int wait_flag_set(volatile uint32_t *reg, uint32_t flag)
{
    uint32_t timeout = I2C_TIMEOUT;

    while ((*reg & flag) == 0u) {
        if (timeout-- == 0u) {
            return 0;
        }
    }

    return 1;
}

static void i2c2_gpio_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;

    /*
     * PB10 I2C2_SCL, PB11 I2C2_SDA:
     * alternate-function open-drain, 2 MHz.
     */
    GPIOB_CRH &= ~((0xFu << 8u) | (0xFu << 12u));
    GPIOB_CRH |=  ((0xEu << 8u) | (0xEu << 12u));
}

static void i2c2_init(void)
{
    i2c2_gpio_init();
    RCC_APB1ENR |= RCC_APB1ENR_I2C2EN;

    I2C2_CR1 = I2C_CR1_SWRST;
    I2C2_CR1 = 0u;

    I2C2_CR2 = 8u;     /* APB1 clock is 8 MHz in this project. */
    I2C2_CCR = 40u;    /* Standard mode: 8 MHz / (2 * 100 kHz). */
    I2C2_TRISE = 9u;   /* 1000 ns / 125 ns + 1. */
    I2C2_CR1 = I2C_CR1_PE;
}

static int i2c2_start_and_addr(uint8_t address, uint8_t read)
{
    I2C2_CR1 |= I2C_CR1_START;
    if (!wait_flag_set(&I2C2_SR1, I2C_SR1_SB)) {
        return 0;
    }

    I2C2_DR = (uint32_t)((address << 1u) | read);
    if (!wait_flag_set(&I2C2_SR1, I2C_SR1_ADDR)) {
        I2C2_CR1 |= I2C_CR1_STOP;
        return 0;
    }

    (void)I2C2_SR1;
    (void)I2C2_SR2;
    return 1;
}

static int i2c2_write_byte(uint8_t value)
{
    if (!wait_flag_set(&I2C2_SR1, I2C_SR1_TXE)) {
        return 0;
    }

    I2C2_DR = value;
    return wait_flag_set(&I2C2_SR1, I2C_SR1_BTF);
}

static int mpu6050_write_reg(uint8_t reg, uint8_t value)
{
    if (!i2c2_start_and_addr(MPU6050_ADDR, 0u)) {
        return 0;
    }
    if (!i2c2_write_byte(reg)) {
        I2C2_CR1 |= I2C_CR1_STOP;
        return 0;
    }
    if (!i2c2_write_byte(value)) {
        I2C2_CR1 |= I2C_CR1_STOP;
        return 0;
    }

    I2C2_CR1 |= I2C_CR1_STOP;
    return 1;
}

static int mpu6050_read_regs(uint8_t reg, uint8_t *data, uint32_t length)
{
    if (length == 0u) {
        return 1;
    }

    if (!i2c2_start_and_addr(MPU6050_ADDR, 0u)) {
        return 0;
    }
    if (!i2c2_write_byte(reg)) {
        I2C2_CR1 |= I2C_CR1_STOP;
        return 0;
    }

    if (length == 1u) {
        I2C2_CR1 &= ~I2C_CR1_ACK;
        if (!i2c2_start_and_addr(MPU6050_ADDR, 1u)) {
            return 0;
        }
        I2C2_CR1 |= I2C_CR1_STOP;
        if (!wait_flag_set(&I2C2_SR1, I2C_SR1_RXNE)) {
            return 0;
        }
        data[0] = (uint8_t)I2C2_DR;
        I2C2_CR1 |= I2C_CR1_ACK;
        return 1;
    }

    I2C2_CR1 |= I2C_CR1_ACK;
    if (!i2c2_start_and_addr(MPU6050_ADDR, 1u)) {
        return 0;
    }

    for (uint32_t i = 0u; i < length; ++i) {
        if (i + 1u == length) {
            I2C2_CR1 &= ~I2C_CR1_ACK;
            I2C2_CR1 |= I2C_CR1_STOP;
        }

        if (!wait_flag_set(&I2C2_SR1, I2C_SR1_RXNE)) {
            I2C2_CR1 |= I2C_CR1_ACK;
            return 0;
        }
        data[i] = (uint8_t)I2C2_DR;
    }

    I2C2_CR1 |= I2C_CR1_ACK;
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

int main(void)
{
    uint8_t who_am_i = 0u;
    uint8_t raw[14];

    OLED_DisplayInit();
    OLED_DisplayTextStatus("MPU6050", "HW I2C2");

    i2c2_init();
    delay_ms(100u);

    while (!mpu6050_read_reg(MPU6050_WHO_AM_I, &who_am_i)) {
        OLED_DisplayTextStatus("MPU6050", "NO ACK");
        delay_ms(500u);
    }

    if (who_am_i != MPU6050_ADDR) {
        OLED_DisplayTextStatus("MPU6050", "BAD ID");
        delay_ms(1000u);
    }

    if (!mpu6050_write_reg(MPU6050_PWR_MGMT_1, 0x00u)) {
        OLED_DisplayTextStatus("MPU6050", "WAKE ERR");
        delay_ms(1000u);
    }
    delay_ms(100u);

    while (1) {
        if (mpu6050_read_regs(MPU6050_ACCEL_XOUT_H, raw, sizeof(raw))) {
            OLED_DisplayMpu6050Raw(read_i16_be(&raw[0]), read_i16_be(&raw[2]),
                                   read_i16_be(&raw[4]), read_i16_be(&raw[8]),
                                   read_i16_be(&raw[10]), read_i16_be(&raw[12]),
                                   read_i16_be(&raw[6]));
        } else {
            OLED_DisplayTextStatus("MPU6050", "READ ERR");
            delay_ms(500u);
            i2c2_init();
        }
        delay_ms(300u);
    }
}
