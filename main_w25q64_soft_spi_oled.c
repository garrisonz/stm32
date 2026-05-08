#include <stdint.h>

#include "oled_display.h"
#include "stm32f1_gpio.h"

#define SPI_PORT             GPIOA
#define W25Q64_CS_PIN        4u
#define W25Q64_CLK_PIN       5u
#define W25Q64_DO_PIN        6u
#define W25Q64_DI_PIN        7u

#define SYSTICK_CSR          (*(volatile uint32_t *)0xE000E010u)
#define SYSTICK_RVR          (*(volatile uint32_t *)0xE000E014u)
#define SYSTICK_CVR          (*(volatile uint32_t *)0xE000E018u)

#define W25Q64_CMD_WRITE_ENABLE  0x06u
#define W25Q64_CMD_READ_STATUS1  0x05u
#define W25Q64_CMD_PAGE_PROGRAM  0x02u
#define W25Q64_CMD_READ_DATA     0x03u
#define W25Q64_CMD_SECTOR_ERASE  0x20u
#define W25Q64_CMD_JEDEC_ID      0x9Fu

#define W25Q64_STATUS_BUSY       (1u << 0)
#define W25Q64_TEST_ADDR         0x7F0000u
#define W25Q64_TEST_SIZE         16u
#define W25Q64_TIMEOUT           500000u

static const uint8_t test_pattern[W25Q64_TEST_SIZE] = {
    0x53u, 0x54u, 0x4Du, 0x33u, 0x32u, 0x2Du, 0x57u, 0x32u,
    0x35u, 0x51u, 0x36u, 0x34u, 0x2Du, 0x53u, 0x50u, 0x49u,
};

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

static void spi_delay(void)
{
    delay_cycles(4u);
}

static void spi_cs_low(void)
{
    GPIO_WritePin(SPI_PORT, W25Q64_CS_PIN, GPIO_PIN_RESET);
}

static void spi_cs_high(void)
{
    GPIO_WritePin(SPI_PORT, W25Q64_CS_PIN, GPIO_PIN_SET);
}

static void spi_clk_low(void)
{
    GPIO_WritePin(SPI_PORT, W25Q64_CLK_PIN, GPIO_PIN_RESET);
}

static void spi_clk_high(void)
{
    GPIO_WritePin(SPI_PORT, W25Q64_CLK_PIN, GPIO_PIN_SET);
}

static void spi_di_write(GPIO_PinState state)
{
    GPIO_WritePin(SPI_PORT, W25Q64_DI_PIN, state);
}

static GPIO_PinState spi_do_read(void)
{
    return GPIO_ReadPin(SPI_PORT, W25Q64_DO_PIN);
}

static void soft_spi_init(void)
{
    RCC_EnableGPIOA();

    GPIO_ConfigOutputPushPull(SPI_PORT, W25Q64_CS_PIN);
    GPIO_ConfigOutputPushPull(SPI_PORT, W25Q64_CLK_PIN);
    GPIO_ConfigInputPullUp(SPI_PORT, W25Q64_DO_PIN);
    GPIO_ConfigOutputPushPull(SPI_PORT, W25Q64_DI_PIN);

    spi_cs_high();
    spi_clk_low();
    spi_di_write(GPIO_PIN_RESET);
}

static uint8_t spi_transfer(uint8_t value)
{
    uint8_t received = 0u;

    for (uint8_t mask = 0x80u; mask != 0u; mask >>= 1u) {
        if ((value & mask) != 0u) {
            spi_di_write(GPIO_PIN_SET);
        } else {
            spi_di_write(GPIO_PIN_RESET);
        }

        spi_delay();
        spi_clk_high();
        spi_delay();

        received <<= 1u;
        if (spi_do_read() == GPIO_PIN_SET) {
            received |= 1u;
        }

        spi_clk_low();
        spi_delay();
    }

    return received;
}

static void w25q64_write_addr(uint32_t addr)
{
    spi_transfer((uint8_t)(addr >> 16u));
    spi_transfer((uint8_t)(addr >> 8u));
    spi_transfer((uint8_t)addr);
}

static void w25q64_read_jedec_id(uint8_t id[3])
{
    spi_cs_low();
    spi_transfer(W25Q64_CMD_JEDEC_ID);
    id[0] = spi_transfer(0xFFu);
    id[1] = spi_transfer(0xFFu);
    id[2] = spi_transfer(0xFFu);
    spi_cs_high();
}

static uint8_t w25q64_read_status1(void)
{
    uint8_t status;

    spi_cs_low();
    spi_transfer(W25Q64_CMD_READ_STATUS1);
    status = spi_transfer(0xFFu);
    spi_cs_high();

    return status;
}

static void w25q64_write_enable(void)
{
    spi_cs_low();
    spi_transfer(W25Q64_CMD_WRITE_ENABLE);
    spi_cs_high();
}

static int w25q64_wait_ready(void)
{
    uint32_t timeout = W25Q64_TIMEOUT;

    while ((w25q64_read_status1() & W25Q64_STATUS_BUSY) != 0u) {
        if (timeout-- == 0u) {
            return 0;
        }
    }

    return 1;
}

static int w25q64_sector_erase(uint32_t addr)
{
    w25q64_write_enable();

    spi_cs_low();
    spi_transfer(W25Q64_CMD_SECTOR_ERASE);
    w25q64_write_addr(addr);
    spi_cs_high();

    return w25q64_wait_ready();
}

static int w25q64_page_program(uint32_t addr, const uint8_t *data,
                               uint32_t length)
{
    w25q64_write_enable();

    spi_cs_low();
    spi_transfer(W25Q64_CMD_PAGE_PROGRAM);
    w25q64_write_addr(addr);
    for (uint32_t i = 0u; i < length; ++i) {
        spi_transfer(data[i]);
    }
    spi_cs_high();

    return w25q64_wait_ready();
}

static void w25q64_read_data(uint32_t addr, uint8_t *data, uint32_t length)
{
    spi_cs_low();
    spi_transfer(W25Q64_CMD_READ_DATA);
    w25q64_write_addr(addr);
    for (uint32_t i = 0u; i < length; ++i) {
        data[i] = spi_transfer(0xFFu);
    }
    spi_cs_high();
}

static int buffer_is_erased(const uint8_t *data, uint32_t length)
{
    for (uint32_t i = 0u; i < length; ++i) {
        if (data[i] != 0xFFu) {
            return 0;
        }
    }

    return 1;
}

static int buffer_equals(const uint8_t *left, const uint8_t *right,
                         uint32_t length)
{
    for (uint32_t i = 0u; i < length; ++i) {
        if (left[i] != right[i]) {
            return 0;
        }
    }

    return 1;
}

int main(void)
{
    uint8_t id[3] = {0u, 0u, 0u};
    uint8_t readback[W25Q64_TEST_SIZE];
    uint8_t pass = 0u;

    OLED_DisplayInit();
    OLED_DisplayTextStatus("W25Q64", "INIT");

    soft_spi_init();
    delay_ms(10u);

    w25q64_read_jedec_id(id);
    OLED_DisplayTextStatus("W25Q64", "ERASE");

    if (w25q64_sector_erase(W25Q64_TEST_ADDR)) {
        w25q64_read_data(W25Q64_TEST_ADDR, readback, sizeof(readback));
        pass = (uint8_t)buffer_is_erased(readback, sizeof(readback));
    }

    if (pass != 0u) {
        OLED_DisplayTextStatus("W25Q64", "WRITE");
        pass = (uint8_t)w25q64_page_program(W25Q64_TEST_ADDR, test_pattern,
                                            sizeof(test_pattern));
    }

    if (pass != 0u) {
        w25q64_read_data(W25Q64_TEST_ADDR, readback, sizeof(readback));
        pass = (uint8_t)buffer_equals(readback, test_pattern, sizeof(readback));
    }

    OLED_DisplayW25q64Test(id[0], id[1], id[2], W25Q64_TEST_ADDR, pass);

    while (1) {
        delay_ms(1000u);
    }
}
