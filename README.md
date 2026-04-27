# STM32F1 PA0 LED 闪烁示例

该工程用于通过 ST-Link V2 给 STM32F1 中容量芯片烧录一个最小裸机程序。当前检测到的目标为 `STM32F1xx_MD`，Flash 64KB，SRAM 20KB。

## 硬件连接

- LED 正极通过限流电阻接开发板 `3.3V`。
- LED 负极接 `A0`，即 STM32 的 `PA0`。
- 这种接法为低电平点亮：`PA0=0` 时 LED 亮，`PA0=1` 时 LED 灭。
- ST-Link 与目标板必须共地，并连接 SWDIO、SWCLK，必要时连接 NRST。

## 构建与烧录

需要安装 ARM 裸机编译器：

```bash
sudo apt-get install -y gcc-arm-none-eabi binutils-arm-none-eabi
```

构建固件：

```bash
make
```

通过 ST-Link 烧录：

```bash
make flash
```

如果需要清理构建产物：

```bash
make clean
```
