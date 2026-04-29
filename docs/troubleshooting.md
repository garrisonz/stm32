# Troubleshooting Notes

## DC motor driver no output

Date: 2026-04-29

Setup:

- Motor driver VM connected to ST-Link V2 5V.
- Motor driver VCC connected to STM32 3.3V.
- Motor driver GND connected to STM32 GND.
- AO1/AO2 connected to the DC motor.
- PWMA connected to STM32 PA2.
- AIN1 connected to STM32 PA4.
- AIN2 connected to STM32 PA5.
- STBY was checked and connected to 3.3V.

Firmware tested:

- `main_dc_motor_button.c`: PB1 button changes speed in 10% steps.
- `main_dc_motor_test.c`: fixed test output with PA4 high, PA5 low, and PA2/TIM2_CH3 at 1 kHz 50% PWM.

Firmware-side check:

- `dc_motor_test` disassembly confirms PA2 is configured as alternate-function push-pull.
- PA4 is driven high.
- PA5 is driven low.
- TIM2_CH3 is enabled with `PSC=79`, `ARR=99`, `CCR3=50`.

Current finding:

- The motor still did not rotate with `flash-dc-motor-test`.
- A pin on the motor driver module appears to have come loose or fallen off, so the circuit may be open.

Next session:

- Repair or replace the motor driver module before continuing firmware debugging.
- After repair, retest with `make flash-dc-motor-test`.
- Measure `VCC`, `VM`, `STBY`, `AIN1`, `AIN2`, and `PWMA` against GND.
- If inputs are correct but AO1/AO2 still have no drive output, suspect the motor driver module or VM power supply.
