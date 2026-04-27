# 仓库指南

## 项目结构与模块组织

当前仓库是一个空的 STM32 工作区。固件项目应按开发板或应用名称放在仓库根目录，例如 `nucleo-f401re/` 或 `motor-control/`。每个项目建议采用以下结构：

- `Core/` 或 `src/` 存放应用源码。
- `Inc/` 或 `include/` 存放公共头文件。
- `Drivers/` 存放厂商 HAL、CMSIS、BSP 或生成的驱动代码。
- `tests/` 存放主机端单元测试或硬件在环测试记录。
- `docs/` 存放原理图、引脚映射、烧录说明和开发板配置。

生成的构建产物应放在 `build/`、`cmake-build-*` 或工具链专用目录中，并排除在版本控制之外。

## 构建、测试与开发命令

当前尚未提交构建系统。添加第一个 STM32 项目时，应补充标准命令。常见示例：

- `cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake` 配置基于 CMake 的固件构建。
- `cmake --build build` 编译固件镜像。
- `make` 构建由 STM32CubeMX 生成的 Makefile 项目。
- `openocd -f interface/stlink.cfg -f target/stm32*.cfg` 启动 ST-Link 调试服务。

每个项目应提交独立的 `README.md`，写明开发板、MCU、工具链和烧录命令。

## 代码风格与命名约定

仅在项目需要时使用 C11 或 C++17。统一使用 4 空格缩进，不使用 Tab，并保持适合嵌入式调试器查看的行宽。C 源文件和头文件使用小写加下划线命名，例如 `uart_dma.c` 和 `motor_control.h`。类型使用 `PascalCase`，宏和常量使用 `UPPER_SNAKE_CASE`，函数和变量使用 `snake_case`，除非厂商代码已有固定约定。

保持 STM32CubeMX 生成区段完整，自定义逻辑应放在 user-code 区块或独立模块中，以减少合并冲突。

## 测试指南

当逻辑可以脱离硬件运行时，在 `tests/` 下添加主机端测试。测试文件使用描述性命名，例如 `test_crc.c` 或 `test_motor_control.cpp`。手动测试或硬件在环验证步骤应记录在 `docs/test-procedure.md`，包括开发板版本、连接外设、预期串口输出和通过标准。

## 提交与 Pull Request 规范

当前目录尚无 Git 历史。提交信息应简短、使用祈使句，例如 `Add UART DMA driver` 或 `Document ST-Link flashing flow`。Pull Request 应说明目标开发板、受影响外设、测试证据和所需硬件配置。若行为只能在硬件上观察，应附截图或串口日志。

## Agent 专用说明

除非明确要重新生成构建系统，否则不要删除厂商生成文件。编辑固件项目前，先检查该项目的本地 README、工具链文件和生成代码边界。
