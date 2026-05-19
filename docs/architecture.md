# Self-Watering Flower Pot V2 Architecture

## Rev A Goal

Rev A is a reliable embedded/mechatronics prototype for a self-watering flower pot. It uses an ESP32-S3 module, a custom PCB, clean connectorized wiring, a protected low-side pump driver, calibrated moisture sensing, a local OLED/encoder UI, and clear fault signaling.

The first build uses a hybrid workflow: Codex prepares documentation, checks, reports, and exports; KiCad GUI remains the source of truth for schematic placement, PCB placement, routing, board outline, and enclosure-sensitive mechanical decisions.

## Power Tree

```text
5V USB-C input
  |
  +-- Pump + bulk input capacitance
  |
  +-- AP63203WU-7 buck regulator
        |
        +-- +3V3 logic rail
              +-- ESP32-S3-WROOM-1-N8
              +-- Moisture sensor power/reference
              +-- OLED / UI logic
              +-- Pullups, LEDs, sensor inputs
```

Design rules:
- USB-C is a 5V sink only for Rev A; do not add USB-PD complexity.
- Add USB-C CC resistors if using a bare USB-C receptacle.
- Keep the buck regulator layout compact and close to its input/output capacitors and inductor.
- Keep pump current loops short and away from moisture ADC routing.
- Include test pads for `+5V`, `+3V3`, and `GND`.

## Functional Blocks

### MCU

Use `ESP32-S3-WROOM-1-N8` as the main controller. Place the module with the antenna at a board edge and respect the keepout. Preserve native USB if practical and include backup UART/debug pads.

### Pump Driver

Use a low-side N-MOSFET switch:

```text
+5V -> PUMP+ connector
PUMP- connector -> MOSFET drain
MOSFET source -> GND
ESP32 GPIO -> gate resistor -> MOSFET gate
MOSFET gate -> pulldown -> GND
Flyback diode across pump, reverse-biased during normal operation
```

Required support:
- AO3400A or equivalent logic-level N-MOSFET.
- 100 ohm to 330 ohm gate resistor.
- 100k gate pulldown.
- SS34 or similar flyback diode.
- Pump connector polarity labels.
- Test pad for `PUMP_GATE`.

### Sensors

The capacitive moisture sensor uses a 3-pin connector: `+3V3`, `GND`, `MOISTURE_ADC`. The ADC signal should use an ESP32-S3 ADC1-capable GPIO and may include a small RC low-pass filter plus a series resistor.

The reservoir switch is included as a 2-pin connector with a pullup or pulldown chosen during schematic capture. Firmware must treat an empty or missing reservoir as a pump lockout.

The flow sensor connector is optional-ready. If populated, `FLOW_PULSE` must be 3.3V-safe before reaching the ESP32.

### User Interface

The UI consists of an SSD1306 I2C OLED, EC11 rotary encoder with push switch, and red error LED. It may live on the main board or a cabled/daughter UI board depending on enclosure fit.

Use I2C pullups to `+3V3`, not `+5V`.

## First-Version KiCad Workflow

1. Build project structure and documentation.
2. Plan schematic sheets, connector pinouts, and ESP32 pins.
3. Place schematic manually in KiCad GUI.
4. Run ERC and save the report in `reports`.
5. Assign and verify footprints.
6. Place PCB manually in KiCad GUI with power, antenna, connector, and enclosure constraints first.
7. Route PCB manually in KiCad GUI.
8. Run DRC and save the report in `reports`.
9. Export fabrication outputs only after ERC/DRC results are acceptable.

## Open Mechanical Decisions

- Main board only vs. main board plus UI daughterboard.
- USB-C location relative to enclosure wall.
- OLED and encoder front-panel location.
- Mounting hole positions and screw/inset hardware.
- Pump and sensor connector access direction.
