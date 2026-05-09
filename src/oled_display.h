#ifndef OLED_DISPLAY_H
#define OLED_DISPLAY_H

#include <stdint.h>

void OLED_DisplayInit(void);
void OLED_DisplayUseSoftwareI2C(uint8_t enable);
void OLED_DisplayFrequency(uint32_t frequency_hz);
void OLED_DisplayMeasurement(uint32_t frequency_hz, uint32_t duty_percent);
void OLED_DisplayAdcVoltage(uint32_t adc_value, uint32_t millivolts);
void OLED_DisplayTextStatus(const char *line1, const char *line2);
void OLED_DisplayMpu6050Raw(int16_t accel_x, int16_t accel_y, int16_t accel_z,
                            int16_t gyro_x, int16_t gyro_y, int16_t gyro_z,
                            int16_t temp_raw);
void OLED_DisplayW25q64Test(uint8_t manufacturer_id, uint8_t memory_type,
                            uint8_t capacity_id, uint32_t test_addr,
                            uint8_t pass);
void OLED_DisplayBkpTest(uint16_t previous, uint16_t current,
                         uint16_t signature, uint8_t pass);
void OLED_DisplayRtcTime(uint32_t year, uint32_t month, uint32_t day,
                         uint32_t hours, uint32_t minutes, uint32_t seconds,
                         uint32_t counter, uint32_t elapsed_ms);
void OLED_DisplayClockStatus(uint32_t sysclk_mhz, uint32_t hclk_mhz,
                             uint32_t pclk1_mhz, uint32_t pclk2_mhz);
void OLED_DisplayRunning(uint8_t visible);
void OLED_DisplaySleepWake(uint32_t wake_count, uint8_t rx_byte);

#endif
