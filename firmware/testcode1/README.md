# testcode1

Safe bring-up firmware for the Board A cost-down hardware.

## Safety behavior

- GPIO4 `PUMP_GATE` is set `LOW` at boot and again every loop.
- The web UI has no pump control.
- The firmware has no pump-control API endpoint.
- Use this firmware for USB, Wi-Fi AP, LED, moisture ADC, and optional input-pad checks only.

## Pins

| Signal | GPIO |
|---|---:|
| `MOISTURE_ADC` | 1 |
| `PUMP_GATE` | 4 |
| `RESERVOIR_SW` / `TP6` | 13 |
| `FLOW_PULSE` / `TP10` | 14 |
| `ERROR_LED` / red `D2` | 15 |
| `STATUS_LED` / green `D3` | 16 |

## Hosted UI

- SSID: `FlowerPot-testcode1`
- Password: `flowerpot1`
- URL: `http://192.168.4.1`

The UI shows:

- moisture raw ADC value
- rolling average
- min/max/span since boot or reset
- rough moisture band: `very dry`, `dry-ish`, `moist`, or `wet`
- flow pulse count
- `TP6` to `TP7/GND` live short status
- `TP10` to `TP9/GND` live short status
- 5-second LED test buttons for red, green, or both LEDs
- reset-stats button for moisture min/max and flow pulse count

## API

- `GET /api/status`
  - Keeps the original bring-up fields: `moisture_adc_raw`, `reservoir_sw_low`, `flow_input_low`, `flow_pulses`, and `pump`.
  - Adds `version`, `moisture_adc_avg`, `moisture_adc_min`, `moisture_adc_max`, `moisture_adc_span`, `moisture_band`, and `samples`.
- `POST /api/flash?led=red|green|both`
  - Flashes the selected LED output for 5 seconds.
- `POST /api/reset-stats`
  - Resets moisture statistics and flow pulse count.

## Moisture calibration notes

These bands are diagnostic only until the sensor is tested in real soil. Current observed values:

| Condition | Approx raw ADC |
|---|---:|
| Air / dry sensor | `3485-3495` |
| Loose lightly wet towel contact | `2650` |
| Pressed wet towel contact | `1900` |

For the common capacitive moisture sensor style, higher ADC readings mean drier and lower readings mean wetter.

Initial diagnostic bands in firmware:

| Band | Raw average |
|---|---:|
| `very dry` | `> 3200` |
| `dry-ish` | `2600-3200` |
| `moist` | `2000-2599` |
| `wet` | `< 2000` |

Do not use these as final watering thresholds without real soil calibration.

## Verification on the first assembled board

- Flashed successfully on `COM3`.
- ESP32-S3 booted `testcode1`.
- Laptop connected to `FlowerPot-testcode1`.
- `http://192.168.4.1/api/status` responded.
- `/api/flash?led=both` accepted the LED test request.
