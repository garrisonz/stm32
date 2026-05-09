#include <stdint.h>

#define RCC_APB2ENR  (*(volatile uint32_t *)0x40021018u)
#define RCC_AHBENR   (*(volatile uint32_t *)0x40021014u)
#define GPIOA_CRH    (*(volatile uint32_t *)0x40010804u)
#define USART1_SR    (*(volatile uint32_t *)0x40013800u)
#define USART1_DR    (*(volatile uint32_t *)0x40013804u)
#define USART1_BRR   (*(volatile uint32_t *)0x40013808u)
#define USART1_CR1   (*(volatile uint32_t *)0x4001380Cu)
#define USART1_CR3   (*(volatile uint32_t *)0x40013814u)
#define DMA1_CCR5    (*(volatile uint32_t *)0x40020058u)
#define DMA1_CNDTR5  (*(volatile uint32_t *)0x4002005Cu)
#define DMA1_CPAR5   (*(volatile uint32_t *)0x40020060u)
#define DMA1_CMAR5   (*(volatile uint32_t *)0x40020064u)

#define RCC_AHBENR_DMA1EN    (1u << 0)
#define RCC_APB2ENR_IOPAEN   (1u << 2)
#define RCC_APB2ENR_USART1EN (1u << 14)

#define USART_SR_TC          (1u << 6)
#define USART_SR_TXE         (1u << 7)
#define USART_CR1_RE         (1u << 2)
#define USART_CR1_TE         (1u << 3)
#define USART_CR1_UE         (1u << 13)
#define USART_CR3_DMAR       (1u << 6)

#define DMA_CCR_EN           (1u << 0)
#define DMA_CCR_CIRC         (1u << 5)
#define DMA_CCR_MINC         (1u << 7)
#define DMA_CCR_PL_HIGH      (2u << 12)

#define RX_BUFFER_SIZE 64u
#define DMA_RX_BUFFER_SIZE 64u

static volatile uint8_t dma_rx_buffer[DMA_RX_BUFFER_SIZE];
static uint32_t dma_rx_tail;

static void usart1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /*
     * PA9  USART1_TX: alternate-function push-pull, 2 MHz.
     * PA10 USART1_RX: floating input.
     */
    GPIOA_CRH &= ~((0xFu << 4u) | (0xFu << 8u));
    GPIOA_CRH |=  ((0xAu << 4u) | (0x4u << 8u));

    USART1_CR1 = 0u;
    USART1_BRR = 0x0341u; /* 8 MHz PCLK2, 9600 baud. */
    USART1_CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

static void usart1_dma_rx_init(void)
{
    RCC_AHBENR |= RCC_AHBENR_DMA1EN;

    DMA1_CCR5 = 0u;
    DMA1_CPAR5 = (uint32_t)(uintptr_t)&USART1_DR;
    DMA1_CMAR5 = (uint32_t)(uintptr_t)dma_rx_buffer;
    DMA1_CNDTR5 = DMA_RX_BUFFER_SIZE;
    DMA1_CCR5 = DMA_CCR_MINC | DMA_CCR_CIRC | DMA_CCR_PL_HIGH;

    USART1_CR3 |= USART_CR3_DMAR;
    DMA1_CCR5 |= DMA_CCR_EN;
}

static void usart1_write_char(char ch)
{
    while ((USART1_SR & USART_SR_TXE) == 0u) {
    }

    USART1_DR = (uint32_t)(uint8_t)ch;
}

static int usart1_dma_read_char(char *ch)
{
    uint32_t dma_rx_head = DMA_RX_BUFFER_SIZE - DMA1_CNDTR5;

    if (dma_rx_head == dma_rx_tail) {
        return 0;
    }

    *ch = (char)dma_rx_buffer[dma_rx_tail++];
    if (dma_rx_tail >= DMA_RX_BUFFER_SIZE) {
        dma_rx_tail = 0u;
    }

    return 1;
}

static void usart1_write_string(const char *text)
{
    while (*text != '\0') {
        usart1_write_char(*text++);
    }

    while ((USART1_SR & USART_SR_TC) == 0u) {
    }
}

static void send_marked_line(const char *line)
{
    usart1_write_string("[stm32 received] ");
    usart1_write_string(line);
    usart1_write_string("\r\n");
}

int main(void)
{
    char buffer[RX_BUFFER_SIZE];
    uint32_t length = 0u;

    usart1_init();
    usart1_dma_rx_init();
    usart1_write_string("STM32 USART1 echo ready\r\n");

    while (1) {
        char ch;

        if (!usart1_dma_read_char(&ch)) {
            continue;
        }

        if (ch == '\r') {
            continue;
        }

        if (ch == '\n') {
            buffer[length] = '\0';
            send_marked_line(buffer);
            length = 0u;
            continue;
        }

        if (length < (RX_BUFFER_SIZE - 1u)) {
            buffer[length++] = ch;
        } else {
            buffer[length] = '\0';
            usart1_write_string("[stm32 received] ");
            usart1_write_string(buffer);
            usart1_write_string("... [truncated]\r\n");
            length = 0u;
        }
    }
}
