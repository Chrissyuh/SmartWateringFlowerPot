# ESP32-S3 Pin Allocation Draft

This is a Rev-A planning table for `ESP32-S3-WROOM-1-N8`. Confirm every selected pin against the exact module datasheet, KiCad symbol, and boot-strapping notes before routing.

Avoid using ESP32 pins that conflict with boot mode, flash/PSRAM, native USB, or module-reserved functions unless the schematic intentionally handles those constraints.

## Draft GPIO Allocation

| Signal | Draft GPIO | Direction | Electrical notes | Firmware notes |
|---|---:|---|---|---|
| `MOISTURE_ADC` | GPIO1 | Input analog | ADC1-capable, 3.3V max, optional RC filter | Average/filter and calibrate dry/wet values |
| `PUMP_GATE` | GPIO4 | Output | Gate resistor plus 100k pulldown required | Default LOW at boot; enforce max runtime |
| `I2C_SDA` | GPIO8 | I/O | Pull up to `+3V3` only | OLED and optional I2C devices |
| `I2C_SCL` | GPIO9 | Output | Pull up to `+3V3` only | OLED and optional I2C devices |
| `ENC_A` | GPIO10 | Input | Pullup and/or debounce | Rotary encoder channel A |
| `ENC_B` | GPIO11 | Input | Pullup and/or debounce | Rotary encoder channel B |
| `ENC_SW` | GPIO12 | Input | Pullup and debounce | Encoder push button |
| `RESERVOIR_SW` | GPIO13 | Input | Pullup/pulldown per switch wiring | Block pump when reservoir is empty |
| `FLOW_PULSE` | GPIO14 | Input | Must be 3.3V-safe; level shift if sensor is 5V | Interrupt-capable pulse count |
| `ERROR_LED` | GPIO15 | Output | LED resistor to suit chosen wiring | Active fault indication |
| `STATUS_LED` | GPIO16 | Output | Optional LED resistor | Heartbeat/status indication |
| `UART_TXD` | GPIO17 | Output | Test pad/header optional | Backup debug/programming |
| `UART_RXD` | GPIO18 | Input | Test pad/header optional | Backup debug/programming |

## Reserved / Special Signals

| Signal | Notes |
|---|---|
| `USB_D+` / `USB_D-` | Preserve native USB pins if used for programming/debug. Route as a controlled, short differential pair when practical. |
| `EN` | Include reset circuit and test pad. |
| `BOOT` / `IO0` | Include boot button or accessible test pad if needed by final programming flow. |
| `+3V3` | Logic rail for ESP32, OLED logic, pullups, and sensors that are 3.3V compatible. |
| `+5V` | Pump rail and USB input rail. Do not feed directly into ESP32 GPIO. |

## Connector Pinout Drafts

### J1 USB-C Power Input

| Pin group | Net |
|---|---|
| VBUS | `+5V` |
| GND | `GND` |
| CC1/CC2 | Sink resistors per USB-C receptacle design |
| D+/D- | Native USB if used; otherwise document no-connect intentionally |

### J2 Pump

| Pin | Net | Notes |
|---:|---|---|
| 1 | `PUMP_PLUS` / `+5V` | Label `PUMP+` |
| 2 | `PUMP_LOW` | To MOSFET drain; label `PUMP-` |

### J3 Moisture Sensor

| Pin | Net | Notes |
|---:|---|---|
| 1 | `+3V3` | Sensor power if sensor is 3.3V compatible |
| 2 | `GND` | Sensor ground |
| 3 | `MOISTURE_ADC` | ADC1 input, 3.3V max |

### J4 Reservoir Switch

| Pin | Net | Notes |
|---:|---|---|
| 1 | `RESERVOIR_SW` | Pullup or pulldown in schematic |
| 2 | `GND` or `+3V3` | Choose to match switch wiring |

Default: wire switch to `GND` and use a pullup to `+3V3`, unless the selected float switch requires otherwise.

### J5 Optional Flow Sensor

| Pin | Net | Notes |
|---:|---|---|
| 1 | `+3V3` or `+5V` | Match selected sensor |
| 2 | `GND` | Sensor ground |
| 3 | `FLOW_PULSE` | Must be level-shifted or divided if sensor output is 5V |

### J6 UI Connector

| Pin | Net | Notes |
|---:|---|---|
| 1 | `+3V3` | UI logic power |
| 2 | `GND` | UI ground |
| 3 | `I2C_SDA` | OLED data |
| 4 | `I2C_SCL` | OLED clock |
| 5 | `ENC_A` | Encoder A |
| 6 | `ENC_B` | Encoder B |
| 7 | `ENC_SW` | Encoder push |
| 8 | `ERROR_LED` | Front-panel fault LED |
| 9 | `STATUS_LED` | Optional status LED or spare GPIO |

## Must-Verify Before Routing

- Confirm the exact ESP32-S3 symbol pin numbers and module-reserved pins.
- Confirm `PUMP_GATE` does not toggle or float high during reset/boot.
- Confirm moisture ADC uses ADC1, not ADC2.
- Confirm I2C pullups are to `+3V3`.
- Confirm all external inputs are 3.3V-safe.
- Confirm every connector pin order matches the real cable/module orientation.
