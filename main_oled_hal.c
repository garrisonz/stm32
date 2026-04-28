#include "stm32f1xx_hal.h"

#define OLED_ADDR (0x3Cu << 1)

I2C_HandleTypeDef hi2c1;

static const uint8_t font5x7[][5] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['!'] = {0x00, 0x00, 0x5F, 0x00, 0x00},
    ['H'] = {0x7F, 0x08, 0x08, 0x08, 0x7F},
    ['W'] = {0x7F, 0x20, 0x18, 0x20, 0x7F},
    ['d'] = {0x38, 0x44, 0x44, 0x48, 0x7F},
    ['e'] = {0x38, 0x54, 0x54, 0x54, 0x18},
    ['l'] = {0x00, 0x41, 0x7F, 0x40, 0x00},
    ['o'] = {0x38, 0x44, 0x44, 0x44, 0x38},
    ['r'] = {0x7C, 0x08, 0x04, 0x04, 0x08},
};

static void Error_Handler(void)
{
    while (1) {
    }
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        Error_Handler();
    }

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                    RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV1;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_0) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef gpio = {0};

    if (hi2c->Instance != I2C1) {
        return;
    }

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_AFIO_REMAP_I2C1_ENABLE();

    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
}

static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
}

static void oled_write(uint8_t control, uint8_t value)
{
    uint8_t data[2] = {control, value};
    HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, data, sizeof(data), HAL_MAX_DELAY);
}

static void oled_cmd(uint8_t command)
{
    oled_write(0x00, command);
}

static void oled_data(uint8_t data)
{
    oled_write(0x40, data);
}

static void oled_set_pos(uint8_t page, uint8_t column)
{
    oled_cmd(0xB0u | page);
    oled_cmd(0x00u | (column & 0x0Fu));
    oled_cmd(0x10u | (column >> 4u));
}

static void oled_clear(void)
{
    for (uint8_t page = 0; page < 8u; ++page) {
        oled_set_pos(page, 0);
        for (uint8_t column = 0; column < 128u; ++column) {
            oled_data(0x00);
        }
    }
}

static void oled_putc(char ch)
{
    const uint8_t *glyph = font5x7[(uint8_t)ch];

    for (uint8_t i = 0; i < 5u; ++i) {
        oled_data(glyph[i]);
    }
    oled_data(0x00);
}

static void oled_print(const char *text)
{
    while (*text != '\0') {
        oled_putc(*text++);
    }
}

static void oled_init(void)
{
    HAL_Delay(100);
    oled_cmd(0xAE);
    oled_cmd(0x20);
    oled_cmd(0x02);
    oled_cmd(0xB0);
    oled_cmd(0xC8);
    oled_cmd(0x00);
    oled_cmd(0x10);
    oled_cmd(0x40);
    oled_cmd(0x81);
    oled_cmd(0x7F);
    oled_cmd(0xA1);
    oled_cmd(0xA6);
    oled_cmd(0xA8);
    oled_cmd(0x3F);
    oled_cmd(0xA4);
    oled_cmd(0xD3);
    oled_cmd(0x00);
    oled_cmd(0xD5);
    oled_cmd(0x80);
    oled_cmd(0xD9);
    oled_cmd(0xF1);
    oled_cmd(0xDA);
    oled_cmd(0x12);
    oled_cmd(0xDB);
    oled_cmd(0x40);
    oled_cmd(0x8D);
    oled_cmd(0x14);
    oled_cmd(0xAF);
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_I2C1_Init();

    oled_init();
    oled_clear();
    oled_set_pos(3, 28);
    oled_print("Hello World!");

    while (1) {
    }
}
