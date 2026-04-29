#include <stdint.h>

#include "stm32f1_gpio.h"

#define OLED_ADDR 0x3Cu
#define SCL_PIN   8u
#define SDA_PIN   9u

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

static void oled_cmd(uint8_t command)
{
    i2c_start();
    i2c_write_byte((OLED_ADDR << 1u) | 0u);
    i2c_write_byte(0x00u);
    i2c_write_byte(command);
    i2c_stop();
}

static void oled_data(uint8_t data)
{
    i2c_start();
    i2c_write_byte((OLED_ADDR << 1u) | 0u);
    i2c_write_byte(0x40u);
    i2c_write_byte(data);
    i2c_stop();
}

static void oled_set_pos(uint8_t page, uint8_t column)
{
    oled_cmd(0xB0u | page);
    oled_cmd(0x00u | (column & 0x0Fu));
    oled_cmd(0x10u | (column >> 4u));
}

static void oled_clear(void)
{
    for (uint8_t page = 0u; page < 8u; ++page) {
        oled_set_pos(page, 0u);
        for (uint8_t column = 0u; column < 128u; ++column) {
            oled_data(0x00u);
        }
    }
}

int main(void)
{
    RCC_EnableGPIOB();
    GPIO_ConfigOutputOpenDrain(GPIOB, SCL_PIN);
    GPIO_ConfigOutputOpenDrain(GPIOB, SDA_PIN);
    scl(GPIO_PIN_SET);
    sda(GPIO_PIN_SET);

    delay(800000u);
    oled_cmd(0xAEu);
    oled_cmd(0x20u);
    oled_cmd(0x02u);
    oled_cmd(0xA4u);
    oled_cmd(0xA6u);
    oled_cmd(0xAEu);
    oled_clear();
    oled_cmd(0xAEu);

    while (1) {
        __asm volatile ("nop");
    }
}
