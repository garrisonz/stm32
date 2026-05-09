#include <stdint.h>

#include "oled_display.h"

#define RCC_APB2ENR          (*(volatile uint32_t *)0x40021018u)
#define GPIOA_CRH            (*(volatile uint32_t *)0x40010804u)
#define GPIOA_BSRR           (*(volatile uint32_t *)0x40010810u)

#define USART1_SR            (*(volatile uint32_t *)0x40013800u)
#define USART1_DR            (*(volatile uint32_t *)0x40013804u)
#define USART1_BRR           (*(volatile uint32_t *)0x40013808u)
#define USART1_CR1           (*(volatile uint32_t *)0x4001380Cu)

#define SCB_SCR              (*(volatile uint32_t *)0xE000ED10u)
#define NVIC_ISER1           (*(volatile uint32_t *)0xE000E104u)

#define RCC_APB2ENR_AFIOEN   (1u << 0)
#define RCC_APB2ENR_IOPAEN   (1u << 2)
#define RCC_APB2ENR_USART1EN (1u << 14)

#define USART_SR_RXNE        (1u << 5)
#define USART_SR_TXE         (1u << 7)
#define USART_SR_TC          (1u << 6)
#define USART_CR1_RE         (1u << 2)
#define USART_CR1_TE         (1u << 3)
#define USART_CR1_RXNEIE     (1u << 5)
#define USART_CR1_UE         (1u << 13)

#define USART1_IRQn          37u

static volatile uint8_t rx_byte;
static volatile uint8_t rx_ready;
static volatile uint32_t wake_count;

static void usart1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN |
                   RCC_APB2ENR_USART1EN;

    /*
     * PA9  USART1_TX -> USB-TTL RX.
     * PA10 USART1_RX <- USB-TTL TX, pull-up input for a stable idle level.
     */
    GPIOA_CRH &= ~((0xFu << 4u) | (0xFu << 8u));
    GPIOA_CRH |=  ((0xAu << 4u) | (0x8u << 8u));
    GPIOA_BSRR = 1u << 10u;

    USART1_CR1 = 0u;
    USART1_BRR = 0x0341u; /* 8 MHz PCLK2, 9600 baud. */
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE |
                 USART_CR1_RXNEIE;

    NVIC_ISER1 = 1u << (USART1_IRQn - 32u);
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

static void enter_sleep_mode(void)
{
    SCB_SCR &= ~(1u << 2); /* SLEEPDEEP=0: Sleep mode, not Stop/Standby. */
    __asm volatile ("wfi");
}

void USART1_IRQHandler(void)
{
    if ((USART1_SR & USART_SR_RXNE) != 0u) {
        rx_byte = (uint8_t)(USART1_DR & 0xFFu);
        rx_ready = 1u;
        ++wake_count;
    }
}

int main(void)
{
    OLED_DisplayInit();
    OLED_DisplayTextStatus("SLEEP USART", "INIT");

    usart1_init();
    usart1_write_string("STM32 sleep USART wakeup ready\r\n");

    OLED_DisplayTextStatus("SLEEP USART", "WAIT RX");

    while (1) {
        while (rx_ready == 0u) {
            enter_sleep_mode();
        }

        usart1_write_string("Wake by USART1 RX: ");
        usart1_write_char((char)rx_byte);
        usart1_write_string("\r\n");

        OLED_DisplaySleepWake(wake_count, rx_byte);
        rx_ready = 0u;
    }
}
