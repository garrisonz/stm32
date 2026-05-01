#include <stdint.h>

extern uint32_t _estack;
extern uint32_t _etext;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

int main(void);

void Reset_Handler(void);
void Default_Handler(void);
void EXTI9_5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TIM2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

__attribute__((section(".isr_vector")))
void (*const vector_table[])(void) = {
    (void (*)(void))(&_estack),
    Reset_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    Default_Handler,
    0,
    0,
    0,
    0,
    Default_Handler,
    Default_Handler,
    0,
    Default_Handler,
    Default_Handler,
    Default_Handler, /* WWDG */
    Default_Handler, /* PVD */
    Default_Handler, /* TAMPER */
    Default_Handler, /* RTC */
    Default_Handler, /* FLASH */
    Default_Handler, /* RCC */
    Default_Handler, /* EXTI0 */
    Default_Handler, /* EXTI1 */
    Default_Handler, /* EXTI2 */
    Default_Handler, /* EXTI3 */
    Default_Handler, /* EXTI4 */
    Default_Handler, /* DMA1 Channel1 */
    Default_Handler, /* DMA1 Channel2 */
    Default_Handler, /* DMA1 Channel3 */
    Default_Handler, /* DMA1 Channel4 */
    Default_Handler, /* DMA1 Channel5 */
    Default_Handler, /* DMA1 Channel6 */
    Default_Handler, /* DMA1 Channel7 */
    Default_Handler, /* ADC1_2 */
    Default_Handler, /* USB HP/CAN TX */
    Default_Handler, /* USB LP/CAN RX0 */
    Default_Handler, /* CAN RX1 */
    Default_Handler, /* CAN SCE */
    EXTI9_5_IRQHandler,
    Default_Handler, /* TIM1 BRK */
    Default_Handler, /* TIM1 UP */
    Default_Handler, /* TIM1 TRG COM */
    Default_Handler, /* TIM1 CC */
    TIM2_IRQHandler,
};

void Reset_Handler(void)
{
    uint32_t *src = &_etext;
    uint32_t *dst = &_sdata;

    while (dst < &_edata) {
        *dst++ = *src++;
    }

    for (dst = &_sbss; dst < &_ebss; ++dst) {
        *dst = 0;
    }

    (void)main();

    while (1) {
    }
}

void Default_Handler(void)
{
    while (1) {
    }
}
