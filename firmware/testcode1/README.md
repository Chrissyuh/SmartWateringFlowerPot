# testcode1

Pump-test firmware for the Board A cost-down hardware.

This firmware is for the first controlled pump trial after the MOSFET has been installed and checked. It is not final autonomous watering firmware.

## Safety behavior

- GPIO4 `PUMP_GATE` starts LOW at boot.
- Pump control is unlocked, but every pump request is capped in firmware at `2000 ms`.
- The pump endpoint accepts only `POST` and requires a confirmation token.
- The web UI shows a first-use browser warning before running the pump button.
- AP mode always stays enabled, even if the ESP32 joins home Wi-Fi.
- Use a dedicated 5 V supply or USB power bank for pump tests. Do not run the pump from a laptop USB port.

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

- AP SSID: `FlowerPot-testcode1`
- AP password: `flowerpot1`
- AP URL: `http://192.168.4.1`

The UI shows:

- pump ready/running state and remaining runtime
- 2-second pump test button with first-use warning
- home Wi-Fi settings form
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
  - Adds Wi-Fi state, pump state, moisture average/min/max/span, `moisture_band`, and sample count.
- `POST /api/pump`
  - Body must include `confirm=pump-test`.
  - Optional `duration_ms` is accepted but clamped to `2000`.
  - Returns HTTP `409` if the pump is already running.
- `POST /api/wifi`
  - Body fields: `ssid`, `password`.
  - Saves credentials in ESP32 NVS and tries to join while keeping the AP online.
- `POST /api/wifi/clear`
  - Clears saved home Wi-Fi credentials.
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

## First pump trial checklist

Before pressing the pump button:

- MOSFET orientation checked.
- `TP4 / PUMP_GATE` measured near `0 V` with the previous pump-disabled firmware.
- Pump flyback diode polarity checked.
- Pump connected to `J2` with correct polarity.
- Board powered from a dedicated 5 V source or USB power bank, not a laptop USB port.
- Water is kept away from the PCB.
