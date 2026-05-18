# Schematic Planning Checklist

Do not begin PCB routing until this checklist is reviewed against the schematic.

## Suggested Sheets Or Sections

1. `Power_Input_Regulation`
2. `MCU_ESP32`
3. `Pump_Driver`
4. `Sensors_Connectors`
5. `User_Interface`
6. `Debug_Testpoints`

For a small first schematic, these may be sections on one sheet instead of separate hierarchical sheets.

## Support Components By Block

### Power Input And Regulation

- USB-C 5V input.
- CC resistors for USB-C sink behavior.
- Optional fuse/polyfuse.
- Bulk capacitance near pump/input.
- AP63203WU-7 buck regulator circuit from datasheet.
- `+5V`, `+3V3`, and `GND` test pads.

### ESP32-S3

- ESP32-S3-WROOM-1-N8 module.
- EN/reset circuit.
- BOOT/IO0 access if required.
- Native USB D+/D- if used.
- UART TX/RX test pads or header.
- Decoupling per module guidance.
- Antenna keepout note.

### Pump Driver

- Pump JST connector.
- AO3400A low-side MOSFET.
- Gate resistor.
- 100k gate pulldown.
- SS34 flyback diode.
- `PUMP_GATE` test pad.
- Clear polarity labels.

### Sensors

- Moisture sensor connector with `+3V3`, `GND`, `MOISTURE_ADC`.
- ADC series resistor and optional RC filter.
- Reservoir switch connector with defined pullup/pulldown.
- Optional flow connector with level shifting if pulse output is 5V.

### User Interface

- OLED I2C header or connector.
- I2C pullups to `+3V3`.
- EC11 encoder.
- Encoder push input.
- Red error LED and resistor.
- Optional status LED.
- UI connector if using a daughterboard.

## ERC Risks To Watch

- Missing power input source flags on `+5V` or `+3V3`.
- Hidden unconnected pins on connectors or modules.
- GPIO connected to 5V signals.
- I2C pullups tied to `+5V`.
- Missing gate pulldown on pump MOSFET.
- Flyback diode reversed or connected to wrong nets.
- Moisture signal on non-ADC or ADC2-only pin.
- Power pins hidden by symbol behavior.
- Ambiguous connector pin order.

## Manual KiCad GUI Placement Order

1. Place power input and regulator first.
2. Place ESP32 module and support parts.
3. Place pump driver and pump connector.
4. Place moisture, reservoir, and optional flow connectors.
5. Place OLED/encoder/UI connector.
6. Place LEDs and test points.
7. Assign footprints and verify every real-world connector footprint.
8. Run ERC before moving to PCB layout.
