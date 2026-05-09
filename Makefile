TARGET ?= blink_pa0_lib
BUILD_DIR := build

CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy

MCU_FLAGS := -mcpu=cortex-m3 -mthumb
CFLAGS := $(MCU_FLAGS) -std=c11 -Wall -Wextra -Werror -Os -ffunction-sections -fdata-sections -Isrc
LDFLAGS := $(MCU_FLAGS) -T linker.ld -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

STARTUP := src/startup_stm32f103.c
GPIO := src/stm32f1_gpio.c
OLED := src/oled_display.c
EXAMPLES := examples

SRCS_BLINK_PA0 := $(STARTUP) $(EXAMPLES)/main.c
SRCS_BLINK_PA0_LIB := $(STARTUP) $(GPIO) $(EXAMPLES)/main_lib.c
SRCS_STOP := $(STARTUP) $(GPIO) $(EXAMPLES)/main_stop.c
SRCS_BUZZER := $(STARTUP) $(GPIO) $(EXAMPLES)/main_buzzer.c
SRCS_BUTTONS := $(STARTUP) $(GPIO) $(EXAMPLES)/main_buttons.c
SRCS_LIGHT_BUZZER := $(STARTUP) $(GPIO) $(EXAMPLES)/main_light_buzzer.c
SRCS_OLED := $(STARTUP) $(GPIO) $(EXAMPLES)/main_oled.c
SRCS_OLED_OFF := $(STARTUP) $(GPIO) $(EXAMPLES)/main_oled_off.c
SRCS_IR_COUNTER_OLED := $(STARTUP) $(GPIO) $(EXAMPLES)/main_ir_counter_oled.c
SRCS_ENCODER_OLED := $(STARTUP) $(GPIO) $(EXAMPLES)/main_encoder_oled.c
SRCS_ENCODER_EXTI_OLED := $(STARTUP) $(GPIO) $(EXAMPLES)/main_encoder_exti_oled.c
SRCS_TIMER_OLED := $(STARTUP) $(GPIO) $(EXAMPLES)/main_timer_oled.c
SRCS_TIMER_OLED_POLL := $(STARTUP) $(GPIO) $(EXAMPLES)/main_timer_oled_poll.c
SRCS_PWM_BREATH_LED := $(STARTUP) $(GPIO) $(EXAMPLES)/main_pwm_breath_led.c
SRCS_SERVO_BUTTON := $(STARTUP) $(GPIO) $(EXAMPLES)/main_servo_button.c
SRCS_SC0017_BUTTON := $(STARTUP) $(GPIO) $(EXAMPLES)/main_sc0017_button.c
SRCS_DC_MOTOR_BUTTON := $(STARTUP) $(GPIO) $(EXAMPLES)/main_dc_motor_button.c
SRCS_DC_MOTOR_TEST := $(STARTUP) $(GPIO) $(EXAMPLES)/main_dc_motor_test.c
SRCS_INPUT_CAPTURE_OLED := $(STARTUP) $(GPIO) $(OLED) $(EXAMPLES)/main_input_capture_oled.c
SRCS_ADC_OLED := $(STARTUP) $(GPIO) $(OLED) $(EXAMPLES)/main_adc_oled.c
SRCS_ADC_IRQ_OLED := $(STARTUP) $(GPIO) $(OLED) $(EXAMPLES)/main_adc_irq_oled.c
SRCS_USART1_TX := $(STARTUP) $(EXAMPLES)/main_usart1_tx.c
SRCS_USART1_ECHO := $(STARTUP) $(EXAMPLES)/main_usart1_echo.c
SRCS_MPU6050_SOFT_I2C := $(STARTUP) $(GPIO) $(EXAMPLES)/main_mpu6050_soft_i2c.c
SRCS_MPU6050_SOFT_I2C_OLED := $(STARTUP) $(GPIO) $(OLED) $(EXAMPLES)/main_mpu6050_soft_i2c_oled.c
SRCS_MPU6050_HW_I2C_OLED := $(STARTUP) $(GPIO) $(OLED) $(EXAMPLES)/main_mpu6050_hw_i2c_oled.c
SRCS_W25Q64_SOFT_SPI_OLED := $(STARTUP) $(GPIO) $(OLED) $(EXAMPLES)/main_w25q64_soft_spi_oled.c
SRCS_W25Q64_HW_SPI_OLED := $(STARTUP) $(GPIO) $(OLED) $(EXAMPLES)/main_w25q64_hw_spi_oled.c
SRCS_BKP_REGISTER_OLED := $(STARTUP) $(GPIO) $(OLED) $(EXAMPLES)/main_bkp_register_oled.c
SRCS_RTC_OLED := $(STARTUP) $(GPIO) $(OLED) $(EXAMPLES)/main_rtc_oled.c
SRCS_CLOCK_CONTROL_OLED := $(STARTUP) $(GPIO) $(OLED) $(EXAMPLES)/main_clock_control_oled.c
SRCS_SLEEP_USART_WAKEUP_OLED := $(STARTUP) $(GPIO) $(OLED) $(EXAMPLES)/main_sleep_usart_wakeup_oled.c
SRCS_STOP_EXTI_PB14_OLED := $(STARTUP) $(GPIO) $(OLED) $(EXAMPLES)/main_stop_exti_pb14_oled.c
SRCS_STANDBY_WKUP_OLED := $(STARTUP) $(GPIO) $(OLED) $(EXAMPLES)/main_standby_wkup_oled.c
SRCS_IWDG_BUTTON_OLED := $(STARTUP) $(GPIO) $(OLED) $(EXAMPLES)/main_iwdg_button_oled.c
SRCS := $(SRCS_$(shell echo $(TARGET) | tr a-z A-Z))
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean flash blink-pa0 blink-pa0-lib stop buzzer buttons light-buzzer oled oled-off ir-counter-oled encoder-oled encoder-exti-oled timer-oled timer-oled-poll pwm-breath-led servo-button sc0017-button dc-motor-button dc-motor-test input-capture-oled adc-oled adc-irq-oled usart1-tx usart1-echo mpu6050-soft-i2c mpu6050-soft-i2c-oled mpu6050-hw-i2c-oled w25q64-soft-spi-oled w25q64-hw-spi-oled bkp-register-oled rtc-oled clock-control-oled sleep-usart-wakeup-oled stop-exti-pb14-oled standby-wkup-oled iwdg-button-oled flash-blink-pa0 flash-blink-pa0-lib flash-stop flash-buzzer flash-buttons flash-light-buzzer flash-oled flash-oled-off flash-ir-counter-oled flash-encoder-oled flash-encoder-exti-oled flash-timer-oled flash-timer-oled-poll flash-pwm-breath-led flash-servo-button flash-sc0017-button flash-dc-motor-button flash-dc-motor-test flash-input-capture-oled flash-adc-oled flash-adc-irq-oled flash-usart1-tx flash-usart1-echo flash-mpu6050-soft-i2c flash-mpu6050-soft-i2c-oled flash-mpu6050-hw-i2c-oled flash-w25q64-soft-spi-oled flash-w25q64-hw-spi-oled flash-bkp-register-oled flash-rtc-oled flash-clock-control-oled flash-sleep-usart-wakeup-oled flash-stop-exti-pb14-oled flash-standby-wkup-oled flash-iwdg-button-oled

all: $(BUILD_DIR)/$(TARGET).bin

blink-pa0:
	$(MAKE) TARGET=blink_pa0

blink-pa0-lib:
	$(MAKE) TARGET=blink_pa0_lib

stop:
	$(MAKE) TARGET=stop

buzzer:
	$(MAKE) TARGET=buzzer

buttons:
	$(MAKE) TARGET=buttons

light-buzzer:
	$(MAKE) TARGET=light_buzzer

oled:
	$(MAKE) TARGET=oled

oled-off:
	$(MAKE) TARGET=oled_off

ir-counter-oled:
	$(MAKE) TARGET=ir_counter_oled

encoder-oled:
	$(MAKE) TARGET=encoder_oled

encoder-exti-oled:
	$(MAKE) TARGET=encoder_exti_oled

timer-oled:
	$(MAKE) TARGET=timer_oled

timer-oled-poll:
	$(MAKE) TARGET=timer_oled_poll

pwm-breath-led:
	$(MAKE) TARGET=pwm_breath_led

servo-button:
	$(MAKE) TARGET=servo_button

sc0017-button:
	$(MAKE) TARGET=sc0017_button

dc-motor-button:
	$(MAKE) TARGET=dc_motor_button

dc-motor-test:
	$(MAKE) TARGET=dc_motor_test

input-capture-oled:
	$(MAKE) TARGET=input_capture_oled

adc-oled:
	$(MAKE) TARGET=adc_oled

adc-irq-oled:
	$(MAKE) TARGET=adc_irq_oled

usart1-tx:
	$(MAKE) TARGET=usart1_tx

usart1-echo:
	$(MAKE) TARGET=usart1_echo

mpu6050-soft-i2c:
	$(MAKE) TARGET=mpu6050_soft_i2c

mpu6050-soft-i2c-oled:
	$(MAKE) TARGET=mpu6050_soft_i2c_oled

mpu6050-hw-i2c-oled:
	$(MAKE) TARGET=mpu6050_hw_i2c_oled

w25q64-soft-spi-oled:
	$(MAKE) TARGET=w25q64_soft_spi_oled

w25q64-hw-spi-oled:
	$(MAKE) TARGET=w25q64_hw_spi_oled

bkp-register-oled:
	$(MAKE) TARGET=bkp_register_oled

rtc-oled:
	$(MAKE) TARGET=rtc_oled

clock-control-oled:
	$(MAKE) TARGET=clock_control_oled

sleep-usart-wakeup-oled:
	$(MAKE) TARGET=sleep_usart_wakeup_oled

stop-exti-pb14-oled:
	$(MAKE) TARGET=stop_exti_pb14_oled

standby-wkup-oled:
	$(MAKE) TARGET=standby_wkup_oled

iwdg-button-oled:
	$(MAKE) TARGET=iwdg_button_oled

flash-blink-pa0:
	$(MAKE) TARGET=blink_pa0 flash

flash-blink-pa0-lib:
	$(MAKE) TARGET=blink_pa0_lib flash

flash-stop:
	$(MAKE) TARGET=stop flash

flash-buzzer:
	$(MAKE) TARGET=buzzer flash

flash-buttons:
	$(MAKE) TARGET=buttons flash

flash-light-buzzer:
	$(MAKE) TARGET=light_buzzer flash

flash-oled:
	$(MAKE) TARGET=oled flash

flash-oled-off:
	$(MAKE) TARGET=oled_off flash

flash-ir-counter-oled:
	$(MAKE) TARGET=ir_counter_oled flash

flash-encoder-oled:
	$(MAKE) TARGET=encoder_oled flash

flash-encoder-exti-oled:
	$(MAKE) TARGET=encoder_exti_oled flash

flash-timer-oled:
	$(MAKE) TARGET=timer_oled flash

flash-timer-oled-poll:
	$(MAKE) TARGET=timer_oled_poll flash

flash-pwm-breath-led:
	$(MAKE) TARGET=pwm_breath_led flash

flash-servo-button:
	$(MAKE) TARGET=servo_button flash

flash-sc0017-button:
	$(MAKE) TARGET=sc0017_button flash

flash-dc-motor-button:
	$(MAKE) TARGET=dc_motor_button flash

flash-dc-motor-test:
	$(MAKE) TARGET=dc_motor_test flash

flash-input-capture-oled:
	$(MAKE) TARGET=input_capture_oled flash

flash-adc-oled:
	$(MAKE) TARGET=adc_oled flash

flash-adc-irq-oled:
	$(MAKE) TARGET=adc_irq_oled flash

flash-usart1-tx:
	$(MAKE) TARGET=usart1_tx flash

flash-usart1-echo:
	$(MAKE) TARGET=usart1_echo flash

flash-mpu6050-soft-i2c:
	$(MAKE) TARGET=mpu6050_soft_i2c flash

flash-mpu6050-soft-i2c-oled:
	$(MAKE) TARGET=mpu6050_soft_i2c_oled flash

flash-mpu6050-hw-i2c-oled:
	$(MAKE) TARGET=mpu6050_hw_i2c_oled flash

flash-w25q64-soft-spi-oled:
	$(MAKE) TARGET=w25q64_soft_spi_oled flash

flash-w25q64-hw-spi-oled:
	$(MAKE) TARGET=w25q64_hw_spi_oled flash

flash-bkp-register-oled:
	$(MAKE) TARGET=bkp_register_oled flash

flash-rtc-oled:
	$(MAKE) TARGET=rtc_oled flash

flash-clock-control-oled:
	$(MAKE) TARGET=clock_control_oled flash

flash-sleep-usart-wakeup-oled:
	$(MAKE) TARGET=sleep_usart_wakeup_oled flash

flash-stop-exti-pb14-oled:
	$(MAKE) TARGET=stop_exti_pb14_oled flash

flash-standby-wkup-oled:
	$(MAKE) TARGET=standby_wkup_oled flash

flash-iwdg-button-oled:
	$(MAKE) TARGET=iwdg_button_oled flash

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(BUILD_DIR)/$(TARGET).bin
	st-flash write $< 0x08000000

clean:
	rm -rf $(BUILD_DIR)
