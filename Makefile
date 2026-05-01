TARGET ?= blink_pa0_lib
BUILD_DIR := build

CC := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy

MCU_FLAGS := -mcpu=cortex-m3 -mthumb
CFLAGS := $(MCU_FLAGS) -std=c11 -Wall -Wextra -Werror -Os -ffunction-sections -fdata-sections
LDFLAGS := $(MCU_FLAGS) -T linker.ld -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

SRCS_BLINK_PA0 := startup_stm32f103.c main.c
SRCS_BLINK_PA0_LIB := startup_stm32f103.c stm32f1_gpio.c main_lib.c
SRCS_STOP := startup_stm32f103.c stm32f1_gpio.c main_stop.c
SRCS_BUZZER := startup_stm32f103.c stm32f1_gpio.c main_buzzer.c
SRCS_BUTTONS := startup_stm32f103.c stm32f1_gpio.c main_buttons.c
SRCS_LIGHT_BUZZER := startup_stm32f103.c stm32f1_gpio.c main_light_buzzer.c
SRCS_OLED := startup_stm32f103.c stm32f1_gpio.c main_oled.c
SRCS_OLED_OFF := startup_stm32f103.c stm32f1_gpio.c main_oled_off.c
SRCS_IR_COUNTER_OLED := startup_stm32f103.c stm32f1_gpio.c main_ir_counter_oled.c
SRCS_ENCODER_OLED := startup_stm32f103.c stm32f1_gpio.c main_encoder_oled.c
SRCS_ENCODER_EXTI_OLED := startup_stm32f103.c stm32f1_gpio.c main_encoder_exti_oled.c
SRCS_TIMER_OLED := startup_stm32f103.c stm32f1_gpio.c main_timer_oled.c
SRCS_TIMER_OLED_POLL := startup_stm32f103.c stm32f1_gpio.c main_timer_oled_poll.c
SRCS_PWM_BREATH_LED := startup_stm32f103.c stm32f1_gpio.c main_pwm_breath_led.c
SRCS_SERVO_BUTTON := startup_stm32f103.c stm32f1_gpio.c main_servo_button.c
SRCS_SC0017_BUTTON := startup_stm32f103.c stm32f1_gpio.c main_sc0017_button.c
SRCS_DC_MOTOR_BUTTON := startup_stm32f103.c stm32f1_gpio.c main_dc_motor_button.c
SRCS_DC_MOTOR_TEST := startup_stm32f103.c stm32f1_gpio.c main_dc_motor_test.c
SRCS_INPUT_CAPTURE_OLED := startup_stm32f103.c stm32f1_gpio.c oled_display.c main_input_capture_oled.c
SRCS := $(SRCS_$(shell echo $(TARGET) | tr a-z A-Z))
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean flash blink-pa0 blink-pa0-lib stop buzzer buttons light-buzzer oled oled-off ir-counter-oled encoder-oled encoder-exti-oled timer-oled timer-oled-poll pwm-breath-led servo-button sc0017-button dc-motor-button dc-motor-test input-capture-oled flash-blink-pa0 flash-blink-pa0-lib flash-stop flash-buzzer flash-buttons flash-light-buzzer flash-oled flash-oled-off flash-ir-counter-oled flash-encoder-oled flash-encoder-exti-oled flash-timer-oled flash-timer-oled-poll flash-pwm-breath-led flash-servo-button flash-sc0017-button flash-dc-motor-button flash-dc-motor-test flash-input-capture-oled

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

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) linker.ld
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

flash: $(BUILD_DIR)/$(TARGET).bin
	st-flash write $< 0x08000000

clean:
	rm -rf $(BUILD_DIR)
