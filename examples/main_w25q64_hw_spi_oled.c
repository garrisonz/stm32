#include <stdint.h>

#include "oled_display.h"
#include "stm32f1_gpio.h"

#define RCC_APB2ENR          (*(volatile uint32_t *)0x40021018u)
#define GPIOA_CRL            (*(volatile uint32_t *)0x40010800u)

#define RCC_APB2ENR_IOPAEN   (1u << 2)
#define RCC_APB2ENR_AFIOEN   (1u << 0)
#define RCC_APB2ENR_SPI1EN   (1u << 12)

#define SPI1_CR1             (*(volatile uint32_t *)0x40013000u)
#define SPI1_SR              (*(volatile uint32_t *)0x40013008u)
#define SPI1_DR              (*(volatile uint32_t *)0x4001300Cu)

#define SPI_CR1_CPHA         (1u << 0)
#define SPI_CR1_CPOL         (1u << 1)
#define SPI_CR1_MSTR         (1u << 2)
#define SPI_CR1_BR_DIV8      (2u << 3)
#define SPI_CR1_SPE          (1u << 6)
#define SPI_CR1_SSI          (1u << 8)
#define SPI_CR1_SSM          (1u << 9)

#define SPI_SR_RXNE          (1u << 0)
#define SPI_SR_TXE           (1u << 1)
#define SPI_SR_BSY           (1u << 7)

#define W25Q64_CS_PIN        4u

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
    0x48u, 0x57u, 0x2Du, 0x53u, 0x50u, 0x49u, 0x2Du, 0x57u,
    0x32u, 0x35u, 0x51u, 0x36u, 0x34u, 0x2Du, 0x4Fu, 0x4Bu,
};

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

static void spi_cs_low(void)
{
    GPIO_WritePin(GPIOA, W25Q64_CS_PIN, GPIO_PIN_RESET);
}

static void spi_cs_high(void)
{
    GPIO_WritePin(GPIOA, W25Q64_CS_PIN, GPIO_PIN_SET);
}

static void spi1_gpio_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;

    /*
     * PA4 CS: GPIO push-pull.
     * PA5 SCK, PA7 MOSI: alternate-function push-pull, 2 MHz.
     * PA6 MISO: floating input.
     */
    GPIOA_CRL &= ~((0xFu << 16u) | (0xFu << 20u) |
                   (0xFu << 24u) | (0xFu << 28u));
    GPIOA_CRL |=  ((0x2u << 16u) | (0xAu << 20u) |
                   (0x4u << 24u) | (0xAu << 28u));

    spi_cs_high();
}

static void spi1_init(void)
{
    spi1_gpio_init();
    RCC_APB2ENR |= RCC_APB2ENR_SPI1EN;

    SPI1_CR1 = 0u;
    SPI1_CR1 = SPI_CR1_MSTR | SPI_CR1_BR_DIV8 | SPI_CR1_SSM |
               SPI_CR1_SSI | SPI_CR1_SPE;
    (void)SPI_CR1_CPHA;
    (void)SPI_CR1_CPOL;
}

static uint8_t spi1_transfer(uint8_t value)
{
    while ((SPI1_SR & SPI_SR_TXE) == 0u) {
    }

    *((volatile uint8_t *)&SPI1_DR) = value;

    while ((SPI1_SR & SPI_SR_RXNE) == 0u) {
    }

    uint8_t received = *((volatile uint8_t *)&SPI1_DR);

    while ((SPI1_SR & SPI_SR_BSY) != 0u) {
    }

    return received;
}

static void w25q64_write_addr(uint32_t addr)
{
    spi1_transfer((uint8_t)(addr >> 16u));
    spi1_transfer((uint8_t)(addr >> 8u));
    spi1_transfer((uint8_t)addr);
}

static void w25q64_read_jedec_id(uint8_t id[3])
{
    spi_cs_low();
    spi1_transfer(W25Q64_CMD_JEDEC_ID);
    id[0] = spi1_transfer(0xFFu);
    id[1] = spi1_transfer(0xFFu);
    id[2] = spi1_transfer(0xFFu);
    spi_cs_high();
}

static uint8_t w25q64_read_status1(void)
{
    uint8_t status;

    spi_cs_low();
    spi1_transfer(W25Q64_CMD_READ_STATUS1);
    status = spi1_transfer(0xFFu);
    spi_cs_high();

    return status;
}

static void w25q64_write_enable(void)
{
    spi_cs_low();
    spi1_transfer(W25Q64_CMD_WRITE_ENABLE);
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
    spi1_transfer(W25Q64_CMD_SECTOR_ERASE);
    w25q64_write_addr(addr);
    spi_cs_high();

    return w25q64_wait_ready();
}

static int w25q64_page_program(uint32_t addr, const uint8_t *data,
                               uint32_t length)
{
    w25q64_write_enable();

    spi_cs_low();
    spi1_transfer(W25Q64_CMD_PAGE_PROGRAM);
    w25q64_write_addr(addr);
    for (uint32_t i = 0u; i < length; ++i) {
        spi1_transfer(data[i]);
    }
    spi_cs_high();

    return w25q64_wait_ready();
}

static void w25q64_read_data(uint32_t addr, uint8_t *data, uint32_t length)
{
    spi_cs_low();
    spi1_transfer(W25Q64_CMD_READ_DATA);
    w25q64_write_addr(addr);
    for (uint32_t i = 0u; i < length; ++i) {
        data[i] = spi1_transfer(0xFFu);
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
    OLED_DisplayTextStatus("W25Q64", "HW SPI1");

    spi1_init();
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
