#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdint.h>

void OLED_DisplayInit(void);
void OLED_DisplayFrequency(uint32_t frequency_hz);
void OLED_DisplayMeasurement(uint32_t frequency_hz, uint32_t duty_percent);

#endif
