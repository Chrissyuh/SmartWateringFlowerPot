# Cost-Down Board-A-Only Redesign

Date: 2026-05-19

This branch preserves `main` as the two-board OLED/encoder version and creates a cheaper first-build variant on `codex/cost-down-board-a-only`.

## Design Scope

- Required board: Board A main controller only.
- Deferred board: Board B UI daughterboard remains in the repo but is not part of this cost-down order.
- Removed required UI: OLED, EC11 encoder, UI cable connector, Board B PCB, UI cable housings, and UI passives.
- Required local UI: two Board A 0805 LEDs, active-high from ESP32 GPIO through 330 ohm resistor to LED to GND.
- Required sensing: moisture sensor connector remains.
- Optional sensing: reservoir switch and flow pulse are solder/test pads only and are disabled by firmware default.

## Board A Changes

- `J6`, `R6`, `R7`, and `R8` are excluded from the cost-down board population.
- `J4` and `J5` are excluded as required JST connectors; their useful nets are exposed as optional pads instead.
- Added `R11`/`D2` for `ERROR_LED`.
- Added `R12`/`D3` for `STATUS_LED`.
- Added optional pads:
  - `TP6`: `RESERVOIR_SW`
  - `TP7`: `GND`
  - `TP8`: `+3V3`
  - `TP9`: `GND`
  - `TP10`: `FLOW_PULSE`
- Board outline, mounting holes, USB-C, ESP32-S3, AP63203 buck, pump connector, moisture connector, debug header, and pump driver power path are unchanged.

## Firmware Defaults

- `RESERVOIR_SENSOR_ENABLED=false`
- `FLOW_SENSOR_ENABLED=false`
- Missing reservoir/flow hardware must not block watering unless explicitly enabled.
- USB-C native ESP32-S3 programming remains the primary upload path.
- `J7` remains the fallback 3.3 V UART/EN/BOOT recovery path.
- Calibration and configuration should use USB serial or compile-time constants for the first cheap version.
- The pump must still enforce a maximum runtime timeout and boot-safe gate behavior.

## Cost Target

`fabrication/bom/cost-down-5-sets.csv` estimates the required five-unit cost-down electronics batch at about `$90.77`, or `$18.15/unit`, before shipping, tax, tariffs, tools, enclosure material, and support accessories.

That estimate uses the cheaper generic capacitive moisture sensor line. If the DFRobot SEN0193 sensor is used instead, add about `$24.25` to the five-unit batch.

## Verification

- Board A ERC: `reports/erc-cost-down-board-a.txt`
- Board A DRC: `reports/drc-cost-down-board-a.txt`
- Board A schematic parity: `reports/drc-cost-down-board-a-parity.txt`

Current result after the redesign:

- ERC: 0 violations
- DRC: 0 violations
- Unconnected pads: 0
- Schematic parity issues: 0

## Ordering Notes

- Upload only `Self-Watering_Flower_Pot_Board_A_cost_down_fab.zip` for this version.
- Do not order Board B for this branch.
- Reservoir and flow parts are not required for the first cheap build.
- Before checkout, refresh live supplier pricing and verify the generic moisture sensor pin order and output range.
