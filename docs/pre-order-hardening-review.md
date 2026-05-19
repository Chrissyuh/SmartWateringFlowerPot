# Pre-Order Hardening Review

Date: 2026-05-19

## Status

Board A and Board B remain CAD-clean after the hardening review:

- Board A ERC: `reports/erc-hardening-board-a.txt`, 0 violations.
- Board A DRC: `reports/drc-hardening-board-a.txt`, 0 violations, 0 unconnected items.
- Board A schematic parity: `reports/drc-hardening-board-a-parity.txt`, 0 violations, 0 unconnected items, 0 schematic parity issues.
- Board B ERC: `board_b_ui/reports/erc-hardening-board-b.txt`, 0 violations.
- Board B DRC: `board_b_ui/reports/drc-hardening-board-b.txt`, 0 violations, 0 unconnected items.
- Board B schematic parity: `board_b_ui/reports/drc-hardening-board-b-parity.txt`, 0 violations, 0 unconnected items, 0 schematic parity issues.
- PCB net audit: `reports/pre-order-hardening-net-audit.txt`.

## Decision

Do not make a last-minute PCB ECO before ordering Rev A unless you explicitly decide to trade schedule for added protection. The current board is acceptable as a careful prototype if these rules are followed:

- Use a dedicated 5 V, >=2 A USB adapter or USB power bank for pump operation.
- Do not run the pump from a laptop USB port.
- First flash and first rail checks happen with the pump disconnected.
- Use current limiting during first power-up and first pump pulses.
- Keep every external sensor signal at ESP32-safe 3.3 V logic or analog levels.

The review found useful Rev A.1/product improvements, but not a reason to disturb the current CAD-clean Rev A package:

- Add a USB input polyfuse or current-limited switch.
- Add USB D+/D- ESD protection, and optionally VBUS TVS protection.
- Add a local board-level 10 uF ESP32 bulk capacitor near U1 `3V3/GND` if layout space allows.
- Add physical BOOT and RESET/EN buttons if J7 will be hard to reach in the enclosure.

## Programming Path

Primary path:

1. Use a real USB-C data cable.
2. Plug Board A into the computer with the pump disconnected.
3. Flash through ESP32-S3 native USB on Board A USB-C.

Fallback path through J7:

1. Use a 3.3 V USB-UART adapter only.
2. Connect adapter `GND`, `TX`, and `RX` to J7. Do not connect any 5 V UART signal.
3. Hold J7 `IO0/BOOT` low.
4. Pulse J7 `EN` low, then release `EN`.
5. Release or keep `IO0/BOOT` low as required by the flashing tool, then upload with ESP-IDF, Arduino, or esptool.

Do not use 5 V UART levels on ESP32 pins.

## USB And Power Budget

The two 5.1k CC resistors are USB-C sink-identification resistors. They do not power the board and they do not limit current. The actual power path is:

- USB-C VBUS provides the `+5V` rail.
- `+5V` feeds the pump connector and the AP63203 buck input.
- AP63203 generates `+3V3` for the ESP32, UI, and sensors.
- The ESP32 controls only the MOSFET gate; the pump current does not flow through an ESP32 GPIO.

Budget the prototype conservatively:

- ESP32-S3 plus Wi-Fi current spikes: plan up to roughly 0.5 A on `+3V3`.
- OLED, sensors, LEDs, and pullups are small compared with the ESP32 and pump.
- The selected pump listing gives about 0.18 A running current, but startup and stall current must be measured on arrival.
- Use 5 V, >=2 A for pump testing so startup surge does not brown out the ESP32.

## Protection And Noise Review

Present on Rev A:

- Pump low-side AO3400A switch.
- 220 ohm pump gate resistor.
- 100k pump gate pulldown.
- SS34 flyback diode to the `+5V` pump rail.
- 100 uF pump bulk capacitor.
- Moisture ADC series resistor and optional filter footprint.
- Backup J7 debug/programming header.

Not present on Rev A:

- USB input fuse or current-limited switch.
- USB data-line ESD array.
- Dedicated board-level ESP32 local bulk capacitor placed right at U1.

Prototype acceptance notes:

- The missing fuse/ESD parts are acceptable for a bench prototype, but this board should not be treated as a finished consumer product.
- If ESP32 brownouts occur during Wi-Fi or pump pulses, first test with a stronger 5 V source and pump disconnected. If the problem remains, add temporary capacitance between J7 `3V3` and `GND` for diagnosis or make a Rev A.1 PCB ECO with a local U1 bulk capacitor.
- If a flow sensor or other external module outputs 5 V pulses, do not connect it directly to ESP32 GPIO. Use a 3.3 V-safe sensor output or add level shifting before populating that option.

## Mechanical And Parts Review

Still confirm before checkout:

- L1 body and pads physically match the selected inductor footprint.
- USB-C exact part suffix matches the KiCad footprint and exits the enclosure correctly.
- OLED module pin order is `GND, VCC, SCL, SDA`.
- EC11 shaft height and knob clearance fit Board B and the front panel.
- JST header orientation and mating cable direction match the actual enclosure.
- Reservoir switch geometry matches the reservoir/base design.
- Board B back-side J1 cable exits in the intended direction.

## Order Gate

Before buying:

1. Upload both Gerber zip files to the PCB manufacturer's viewer.
2. Inspect outline, drill, solder mask, silkscreen, copper pours, USB-C pads, antenna keepout, and pin-1 labels.
3. Refresh live BOM stock and prices.
4. Decide explicitly that Rev A is accepted without USB fuse/ESD/local-U1-bulk ECO.
5. Buy a 5 V, >=2 A USB power source, a real USB-C data cable, and a 3.3 V USB-UART adapter for fallback programming.

## References

- AP63203: https://www.diodes.com/part/view/AP63203
- AP63203 DigiKey listing: https://www.digikey.com/en/products/detail/diodes-incorporated/AP63203WU-7/9858426
- ESP32-S3 boot mode selection: https://docs.espressif.com/projects/esptool/en/latest/esp32s3/advanced-topics/boot-mode-selection.html
- ESP32-S3 serial connection and native USB notes: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/get-started/establish-serial-connection.html
- GCT USB4105 spec: https://gct.co/files/specs/usb4105-spec.pdf
- Gikfun pump listing: https://gikfun.com/products/gikfun-dc-3v-5v-micro-submersible-mini-water-pump-pack-of-4pcs
