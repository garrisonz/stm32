# STM32F103 bare-metal examples

这个仓库是一个面向 STM32F103C8T6/STM32F1xx_MD 的裸机练习工程集合，使用 `arm-none-eabi-gcc`、自定义启动文件和链接脚本构建，通过 ST-Link 烧录。代码以寄存器直接配置为主，逐步覆盖 GPIO、OLED、定时器、ADC、USART、I2C、SPI、RTC、低功耗和看门狗等外设。

## 构建与烧录

安装工具链：

```bash
sudo apt-get install -y gcc-arm-none-eabi binutils-arm-none-eabi stlink-tools
```

构建默认目标：

```bash
make
```

构建指定示例：

```bash
make iwdg-button-oled
```

编译并烧录指定示例：

```bash
make flash-iwdg-button-oled
```

如果目标 MCU 处于低功耗或看门狗复位循环，普通烧录连不上时可使用：

```bash
st-flash --connect-under-reset write build/iwdg_button_oled.bin 0x08000000
```

清理构建产物：

```bash
make clean
```

## 目录与公共文件

| 路径 | 简介 |
| --- | --- |
| `Makefile` | 统一管理所有示例的编译和烧录目标。 |
| `src/` | 公共源码目录，存放启动文件、GPIO helper 和 OLED 驱动。 |
| `examples/` | 示例程序目录，存放每个实验的 `main_*.c` 入口文件。 |
| `src/startup_stm32f103.c` | Cortex-M3 启动代码和中断向量表。 |
| `linker.ld` | STM32F103 64KB Flash/20KB SRAM 链接脚本。 |
| `src/stm32f1_gpio.c/.h` | GPIO 时钟、输入、输出和复用功能的基础封装。 |
| `src/oled_display.c/.h` | SSD1306 OLED 显示驱动，支持软件 I2C、硬件 I2C 和 framebuffer 刷新。 |
| `build/` | 编译输出目录，生成 `.o`、`.elf`、`.bin`、`.map` 文件。 |
| `docs/`、`resource/` | 文档和资源资料目录。 |

## 示例程序

| Make 目标 | 主文件 | 一句话简介 |
| --- | --- | --- |
| `blink-pa0` | `examples/main.c` | 最小 PA0 LED 闪烁示例。 |
| `blink-pa0-lib` | `examples/main_lib.c` | 使用 GPIO helper 的 PA0 LED 闪烁示例。 |
| `stop` | `examples/main_stop.c` | 基础低功耗停止模式示例。 |
| `buzzer` | `examples/main_buzzer.c` | GPIO 驱动蜂鸣器。 |
| `buttons` | `examples/main_buttons.c` | GPIO 按键输入读取。 |
| `light-buzzer` | `examples/main_light_buzzer.c` | 光敏传感器联动蜂鸣器。 |
| `oled` | `examples/main_oled.c` | OLED 基础显示。 |
| `oled-off` | `examples/main_oled_off.c` | OLED 关闭/显示控制示例。 |
| `ir-counter-oled` | `examples/main_ir_counter_oled.c` | 对射式红外传感器计次并显示到 OLED。 |
| `encoder-oled` | `examples/main_encoder_oled.c` | 旋转编码器轮询计数并显示。 |
| `encoder-exti-oled` | `examples/main_encoder_exti_oled.c` | 旋转编码器 EXTI 中断计数并显示。 |
| `timer-oled` | `examples/main_timer_oled.c` | 定时器中断计时并显示。 |
| `timer-oled-poll` | `examples/main_timer_oled_poll.c` | 定时器轮询方式计时并显示。 |
| `pwm-breath-led` | `examples/main_pwm_breath_led.c` | PWM LED 呼吸灯。 |
| `servo-button` | `examples/main_servo_button.c` | 按键控制舵机 PWM 输出。 |
| `sc0017-button` | `examples/main_sc0017_button.c` | SC0017 模块按键控制示例。 |
| `dc-motor-button` | `examples/main_dc_motor_button.c` | 按键控制直流电机。 |
| `dc-motor-test` | `examples/main_dc_motor_test.c` | 直流电机测试程序。 |
| `input-capture-oled` | `examples/main_input_capture_oled.c` | 输入捕获测频率/占空比并显示。 |
| `adc-oled` | `examples/main_adc_oled.c` | ADC 采样并显示到 OLED。 |
| `adc-irq-oled` | `examples/main_adc_irq_oled.c` | ADC 中断采样并显示到 OLED。 |
| `usart1-tx` | `examples/main_usart1_tx.c` | USART1 发送文本。 |
| `usart1-echo` | `examples/main_usart1_echo.c` | USART1 DMA 接收回显。 |
| `mpu6050-soft-i2c` | `examples/main_mpu6050_soft_i2c.c` | 软件 I2C 读取 MPU6050 并通过串口输出。 |
| `mpu6050-soft-i2c-oled` | `examples/main_mpu6050_soft_i2c_oled.c` | 软件 I2C 读取 MPU6050 并显示到 OLED。 |
| `mpu6050-hw-i2c-oled` | `examples/main_mpu6050_hw_i2c_oled.c` | 硬件 I2C 读取 MPU6050 并显示到 OLED。 |
| `w25q64-soft-spi-oled` | `examples/main_w25q64_soft_spi_oled.c` | 软件 SPI 读写 W25Q64 并显示测试结果。 |
| `w25q64-hw-spi-oled` | `examples/main_w25q64_hw_spi_oled.c` | 硬件 SPI 读写 W25Q64 并显示测试结果。 |
| `bkp-register-oled` | `examples/main_bkp_register_oled.c` | BKP 备份寄存器读写测试。 |
| `rtc-oled` | `examples/main_rtc_oled.c` | RTC 年月日时分秒、计数器和毫秒显示。 |
| `clock-control-oled` | `examples/main_clock_control_oled.c` | 主频切换演示并显示运行状态。 |
| `sleep-usart-wakeup-oled` | `examples/main_sleep_usart_wakeup_oled.c` | Sleep 模式下 USART1 RX 中断唤醒。 |
| `stop-exti-pb14-oled` | `examples/main_stop_exti_pb14_oled.c` | STOP 模式下 PB14/EXTI 唤醒并用 OLED 显示计数。 |
| `standby-wkup-oled` | `examples/main_standby_wkup_oled.c` | Standby 模式下 PA0/WKUP 唤醒并用 BKP 记录次数。 |
| `iwdg-button-oled` | `examples/main_iwdg_button_oled.c` | 独立看门狗 IWDG 演示，PB1 按键可模拟程序阻塞复位。 |

每个示例对应的烧录目标一般是在构建目标前加 `flash-`，例如：

```bash
make flash-rtc-oled
make flash-stop-exti-pb14-oled
make flash-iwdg-button-oled
```

## 硬件备注

- ST-Link 与目标板必须共地，并连接 `SWDIO`、`SWCLK`，必要时连接 `NRST/RST`。
- OLED 当前默认使用 PB8/PB9；低功耗相关示例通常强制软件 I2C，避免 STOP/Standby 唤醒后硬件 I2C 状态异常。
- `stop-exti-pb14-oled` 使用 PB14 作为遮挡传感器/EXTI 唤醒输入。
- `standby-wkup-oled` 使用 PA0/WKUP 上升沿唤醒，Standby 唤醒后程序会从复位入口重新开始。
- `iwdg-button-oled` 使用 PB1 输入上拉，按钮另一端接 GND。
