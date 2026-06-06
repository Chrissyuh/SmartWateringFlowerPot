# testcode1

First bring-up firmware for Board A cost-down hardware.

Safety behavior:

- GPIO4 `PUMP_GATE` is set `LOW` at boot and again every loop.
- The web UI intentionally has no pump control.
- Use this only for USB, Wi-Fi, LED, ADC, and optional input-pad checks.

Pins:

| Signal | GPIO |
|---|---:|
| `MOISTURE_ADC` | 1 |
| `PUMP_GATE` | 4 |
| `RESERVOIR_SW` / `TP6` | 13 |
| `FLOW_PULSE` / `TP10` | 14 |
| `ERROR_LED` / red `D2` | 15 |
| `STATUS_LED` / green `D3` | 16 |

Hosted UI:

- SSID: `FlowerPot-testcode1`
- Password: `flowerpot1`
- URL: `http://192.168.4.1`

The page has buttons to flash the red LED, green LED, or both LEDs for 5 seconds.

The page also shows live short-detect status for:

- `TP6` to `TP7/GND`
- `TP10` to `TP9/GND`

Verification on the first assembled board:

- Flashed successfully on `COM3`.
- ESP32-S3 booted `testcode1`.
- Laptop connected to `FlowerPot-testcode1`.
- `http://192.168.4.1/api/status` responded.
- `/api/flash?led=both` accepted the LED test request.
