# 故障排查记录

## 直流电机驱动无输出

日期：2026-04-29

接线：

- 电机驱动 VM 连接到 ST-Link V2 的 5V。
- 电机驱动 VCC 连接到 STM32 的 3.3V。
- 电机驱动 GND 连接到 STM32 GND。
- AO1/AO2 连接到直流电机。
- PWMA 连接到 STM32 PA2。
- AIN1 连接到 STM32 PA4。
- AIN2 连接到 STM32 PA5。
- 已检查 STBY，并确认连接到 3.3V。

已测试固件：

- `main_dc_motor_button.c`：PB1 按键以 10% 为步进改变速度。
- `main_dc_motor_test.c`：固定测试输出，PA4 高电平、PA5 低电平，PA2/TIM2_CH3 输出 1 kHz、50% PWM。

固件侧检查：

- `dc_motor_test` 反汇编确认 PA2 已配置为复用推挽输出。
- PA4 输出高电平。
- PA5 输出低电平。
- TIM2_CH3 已启用，配置为 `PSC=79`、`ARR=99`、`CCR3=50`。

当前发现：

- 使用 `flash-dc-motor-test` 后电机仍未转动。
- 电机驱动模块上似乎有一个引脚松动或脱落，因此电路可能已经开路。

下次继续：

- 继续调试固件前，先修复或更换电机驱动模块。
- 修复后，重新执行 `make flash-dc-motor-test` 测试。
- 以 GND 为参考测量 `VCC`、`VM`、`STBY`、`AIN1`、`AIN2` 和 `PWMA`。
- 如果输入信号正确，但 AO1/AO2 仍没有驱动输出，优先怀疑电机驱动模块或 VM 电源。

## PWM 输入捕获占空比少 1%

日期：2026-04-30

现象：

- `main_input_capture_oled.c` 中设置 `TIM2_CCR1 = 70u`。
- `TIM2_ARR = 99u`，理论上 PWM 周期为 100 个计数，占空比应约为 70%。
- OLED 实际显示占空比为 69%。

当前分析：

- `capture_duty_percent()` 中当前写法为 `period_counts = TIM3_CCR1 + 1u`，但 `high_counts = TIM3_CCR2`。
- 如果输入捕获得到 `TIM3_CCR1 = 99`、`TIM3_CCR2 = 69`，计算结果就是 `69 / 100 = 69%`。
- 将 `high_counts` 改为 `TIM3_CCR2 + 1u` 后，OLED 可以显示 70%。
- 暂时理解为周期计数和高电平计数的边界口径不一致，具体原因后续再结合 STM32 定时器输入捕获机制研究。

后续待处理：

- 暂不直接修改最终逻辑。
- 后续查阅 STM32 定时器 PWM 输入模式和捕获寄存器计数边界说明。
- 确认 `TIM3_CCR1 + 1u` 和 `TIM3_CCR2 + 1u` 是否应保持一致，避免其它占空比下引入误差。
