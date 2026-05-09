#include <stdint.h>

#include "stm32f1_gpio.h"

#define SERVO_PIN  2u
#define BUTTON_PIN 1u

#define FLASH_ACR (*(volatile uint32_t *)0x40022000u)
#define FLASH_ACR_LATENCY_2 (2u << 0)
#define FLASH_ACR_PRFTBE    (1u << 4)

#define RCC_CR              (*(volatile uint32_t *)0x40021000u)
#define RCC_CFGR            (*(volatile uint32_t *)0x40021004u)
#define RCC_APB1ENR         (*(volatile uint32_t *)0x4002101Cu)
#define RCC_CR_PLLON        (1u << 24)
#define RCC_CR_PLLRDY       (1u << 25)
#define RCC_CFGR_SW_PLL     (2u << 0)
#define RCC_CFGR_SWS_PLL    (2u << 2)
#define RCC_CFGR_PPRE1_DIV2 (4u << 8)
#define RCC_CFGR_PLLMUL16   (14u << 18)
#define RCC_APB1ENR_USART2EN (1u << 17)

#define USART2_SR  (*(volatile uint32_t *)0x40004400u)
#define USART2_DR  (*(volatile uint32_t *)0x40004404u)
#define USART2_BRR (*(volatile uint32_t *)0x40004408u)
#define USART2_CR1 (*(volatile uint32_t *)0x4000440Cu)

#define USART_SR_TXE  (1u << 7)
#define USART_SR_TC   (1u << 6)
#define USART_CR1_UE  (1u << 13)
#define USART_CR1_TE  (1u << 3)

#define SC0017_ID_BROADCAST 0xFEu
#define SC0017_INST_WRITE   0x03u
#define SC0017_ADDR_TORQUE_ENABLE 0x28u
#define SC0017_ADDR_GOAL_POSITION 0x2Au

#define SC0017_MAX_DEG 210u
#define CONTROL_MAX_DEG 180u
#define CONTROL_STEP_DEG 30u
#define MOVE_SPEED 500u

static void delay(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm volatile ("nop");
    }
}

static void clock_init_64mhz(void)
{
    FLASH_ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

    RCC_CR &= ~RCC_CR_PLLON;
    while ((RCC_CR & RCC_CR_PLLRDY) != 0u) {
    }

    /*
     * HSI is 8 MHz. STM32F1 feeds HSI/2 into the PLL, so PLL x16 gives
     * SYSCLK=64 MHz. APB1 is divided by 2, so USART2 gets PCLK1=32 MHz.
     */
    RCC_CFGR = RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PLLMUL16;
    RCC_CR |= RCC_CR_PLLON;
    while ((RCC_CR & RCC_CR_PLLRDY) == 0u) {
    }

    RCC_CFGR = (RCC_CFGR & ~0x3u) | RCC_CFGR_SW_PLL;
    while ((RCC_CFGR & (3u << 2)) != RCC_CFGR_SWS_PLL) {
    }
}

static void usart2_tx_pa2_init(void)
{
    volatile uint32_t *gpioa_crl = &GPIOA->CRL;

    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    *gpioa_crl &= ~(0xFu << (SERVO_PIN * 4u));
    *gpioa_crl |=  (0xBu << (SERVO_PIN * 4u)); /* PA2: AF push-pull, 50 MHz. */

    USART2_CR1 = 0u;
    USART2_BRR = 0x0020u; /* 32 MHz APB1 / 1 Mbps. */
    USART2_CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void usart2_write_byte(uint8_t value)
{
    while ((USART2_SR & USART_SR_TXE) == 0u) {
    }
    USART2_DR = value;
}

static void usart2_wait_tx_done(void)
{
    while ((USART2_SR & USART_SR_TC) == 0u) {
    }
}

static void sc0017_write(uint8_t address, const uint8_t *data, uint8_t data_len)
{
    uint8_t length = data_len + 3u;
    uint16_t sum = SC0017_ID_BROADCAST + length + SC0017_INST_WRITE + address;

    usart2_write_byte(0xFFu);
    usart2_write_byte(0xFFu);
    usart2_write_byte(SC0017_ID_BROADCAST);
    usart2_write_byte(length);
    usart2_write_byte(SC0017_INST_WRITE);
    usart2_write_byte(address);

    for (uint8_t i = 0u; i < data_len; ++i) {
        sum += data[i];
        usart2_write_byte(data[i]);
    }

    usart2_write_byte((uint8_t)(~sum));
    usart2_wait_tx_done();
}

static void sc0017_enable_torque(void)
{
    uint8_t enable = 1u;

    sc0017_write(SC0017_ADDR_TORQUE_ENABLE, &enable, 1u);
}

static uint16_t sc0017_position_from_degrees(uint32_t angle_deg)
{
    return (uint16_t)((angle_deg * 1023u) / SC0017_MAX_DEG);
}

static void sc0017_set_angle(uint32_t angle_deg)
{
    uint16_t position = sc0017_position_from_degrees(angle_deg);
    uint8_t data[6] = {
        (uint8_t)(position & 0xFFu),
        (uint8_t)(position >> 8u),
        0x00u,
        0x00u,
        (uint8_t)(MOVE_SPEED & 0xFFu),
        (uint8_t)(MOVE_SPEED >> 8u),
    };

    sc0017_write(SC0017_ADDR_GOAL_POSITION, data, sizeof(data));
}

static uint8_t button_is_pressed(void)
{
    return GPIO_ReadPin(GPIOB, BUTTON_PIN) == GPIO_PIN_RESET;
}

int main(void)
{
    uint32_t angle = 0u;
    uint8_t was_pressed = 0u;

    clock_init_64mhz();
    RCC_EnableGPIOA();
    RCC_EnableGPIOB();
    usart2_tx_pa2_init();
    GPIO_ConfigInputPullUp(GPIOB, BUTTON_PIN);

    delay(800000u);
    sc0017_enable_torque();
    sc0017_set_angle(angle);

    while (1) {
        uint8_t pressed = button_is_pressed();

        if (pressed && !was_pressed) {
            delay(50000u);
            if (button_is_pressed()) {
                angle += CONTROL_STEP_DEG;
                if (angle > CONTROL_MAX_DEG) {
                    angle = 0u;
                }
                sc0017_set_angle(angle);
                was_pressed = 1u;
            }
        } else if (!pressed) {
            was_pressed = 0u;
        }

        delay(2000u);
    }
}
