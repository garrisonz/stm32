#include <stdint.h>

#define RCC_APB2ENR (*(volatile uint32_t *)0x40021018u)
#define GPIOA_CRL   (*(volatile uint32_t *)0x40010800u)
#define GPIOA_BSRR  (*(volatile uint32_t *)0x40010810u)
#define GPIOA_BRR   (*(volatile uint32_t *)0x40010814u)

#define RCC_APB2ENR_IOPAEN (1u << 2)
#define LED_PIN            0u
#define PWM_STEPS          100u
#define PWM_CYCLES         3u

static void delay(volatile uint32_t cycles)
{
    while (cycles--) {
        __asm volatile ("nop");
    }
}

static void gpio_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN;

    /* PA0: general-purpose push-pull output, 2 MHz. */
    GPIOA_CRL &= ~(0xFu << (LED_PIN * 4u));
    GPIOA_CRL |=  (0x2u << (LED_PIN * 4u));
}

int main(void)
{
    gpio_init();
    uint32_t brightness = 0;
    int32_t direction = 1;

    while (1) {
        for (uint32_t cycle = 0; cycle < PWM_CYCLES; ++cycle) {
            GPIOA_BRR = (1u << LED_PIN);   /* LED on: PA0 sinks current. */
            delay(brightness * 25u);
            GPIOA_BSRR = (1u << LED_PIN);  /* LED off. */
            delay((PWM_STEPS - brightness) * 25u);
        }

        if (direction > 0) {
            ++brightness;
            if (brightness >= PWM_STEPS) {
                brightness = PWM_STEPS;
                direction = -1;
            }
        } else {
            --brightness;
            if (brightness == 0) {
                direction = 1;
            }
        }
    }
}
