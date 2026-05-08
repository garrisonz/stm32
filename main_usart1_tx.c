#include <stdint.h>

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
#define USART_CR1_UE         (1u << 13)
#define USART_CR1_TE         (1u << 3)
#define USART_CR1_RE         (1u << 2)

#define SYSTICK_CSR   (*(volatile uint32_t *)0xE000E010u)
#define SYSTICK_RVR   (*(volatile uint32_t *)0xE000E014u)
#define SYSTICK_CVR   (*(volatile uint32_t *)0xE000E018u)

static void delay_ms(uint32_t ms)
{
    /* 配置 SysTick 定时器（8MHz 时钟，1ms 中断） */
    SYSTICK_RVR = 8000u - 1u;
    SYSTICK_CVR = 0u;
    SYSTICK_CSR = 0x03u;  /* 使能，使用处理器时钟 */
    
    while (ms--) {
        while ((SYSTICK_CSR & (1u << 16)) == 0u) {  /* 等待 COUNT 标志 */
        }
    }
    
    SYSTICK_CSR = 0u;  /* 禁用 SysTick */
}

static void usart1_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /*
     * PA9  USART1_TX: alternate-function push-pull, 2 MHz.
     * PA10 USART1_RX: floating input.
     */
    GPIOA_CRH &= ~((0xFu << 4u) | (0xFu << 8u));
    GPIOA_CRH |=  ((0xAu << 4u) | (0x4u << 8u));

    USART1_CR1 = 0;
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

static void usart1_write_uint32(uint32_t value)
{
    char digits[10];
    uint32_t index = 0;

    if (value == 0u) {
        usart1_write_char('0');
        return;
    }

    while (value > 0u) {
        digits[index++] = (char)('0' + (value % 10u));
        value /= 10u;
    }

    while (index > 0u) {
        usart1_write_char(digits[--index]);
    }
}

int main(void)
{
    usart1_init();
    uint32_t i = 0;

    while (1) {
        usart1_write_string("Hello from STM32F103 USART1 PA9 -> CH340 RX, i=");
        usart1_write_uint32(i++);
        usart1_write_string("\r\n");
        delay_ms(100);  /* 精确 100ms 延时 */
    }
}
