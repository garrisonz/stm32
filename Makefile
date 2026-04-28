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
SRCS := $(SRCS_$(shell echo $(TARGET) | tr a-z A-Z))
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean flash blink-pa0 blink-pa0-lib stop buzzer buttons light-buzzer oled flash-blink-pa0 flash-blink-pa0-lib flash-stop flash-buzzer flash-buttons flash-light-buzzer flash-oled

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
