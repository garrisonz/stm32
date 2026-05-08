#include <stdint.h>

#include "oled_display.h"
#include "stm32f1_gpio.h"

#define SYSTICK_CSR          (*(volatile uint32_t *)0xE000E010u)
#define SYSTICK_RVR          (*(volatile uint32_t *)0xE000E014u)
#define SYSTICK_CVR          (*(volatile uint32_t *)0xE000E018u)

#define MPU_I2C_PORT         GPIOB
#define MPU_I2C_SCL_PIN      10u
#define MPU_I2C_SDA_PIN      11u

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

static void i2c_delay(void)
{
    delay_cycles(20u);
}

static void i2c_scl_high(void)
{
    GPIO_WritePin(MPU_I2C_PORT, MPU_I2C_SCL_PIN, GPIO_PIN_SET);
}

static void i2c_scl_low(void)
{
    GPIO_WritePin(MPU_I2C_PORT, MPU_I2C_SCL_PIN, GPIO_PIN_RESET);
}

static void i2c_sda_high(void)
{
    GPIO_WritePin(MPU_I2C_PORT, MPU_I2C_SDA_PIN, GPIO_PIN_SET);
}

static void i2c_sda_low(void)
{
    GPIO_WritePin(MPU_I2C_PORT, MPU_I2C_SDA_PIN, GPIO_PIN_RESET);
}

static GPIO_PinState i2c_sda_read(void)
{
    return GPIO_ReadPin(MPU_I2C_PORT, MPU_I2C_SDA_PIN);
}

static void soft_i2c_init(void)
{
    RCC_EnableGPIOB();
    GPIO_ConfigOutputOpenDrain(MPU_I2C_PORT, MPU_I2C_SCL_PIN);
    GPIO_ConfigOutputOpenDrain(MPU_I2C_PORT, MPU_I2C_SDA_PIN);
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

int main(void)
{
    uint8_t who_am_i = 0u;
    uint8_t raw[14];

    OLED_DisplayInit();
    OLED_DisplayTextStatus("MPU6050", "INIT");

    soft_i2c_init();
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
        }
        delay_ms(300u);
    }
}
