# Smart Pot V2 BOM Notes

This is a planning BOM, not a final purchasing list. Confirm exact part numbers, footprints, voltage ratings, current ratings, and availability before ordering.

## Preferred Rev-A Parts

| Function | Preferred part / style | Notes |
|---|---|---|
| MCU | ESP32-S3-WROOM-1-N8 | Module, not dev board. Confirm footprint and antenna keepout. |
| Input power | USB-C 5V sink | Include CC resistors if using a bare receptacle. |
| 3.3V regulator | AP63203WU-7 | Check datasheet layout, inductor, feedback, and package. |
| Buck inductor | 3.9uH shielded power inductor | Confirm saturation current, DCR, pad fit, and height. |
| Buck output caps | 2 x 22uF 6.3V X7R 0805 | Place both close to the AP63203 output path. |
| Pump switch | AO3400A N-MOSFET | Confirm current and thermal margin for selected pump. |
| Flyback diode | SS34 or similar | Across pump/load path, orientation critical. |
| Pump rail bulk cap | 100uF 10V radial electrolytic or equivalent | Place near pump connector; observe polarity. |
| Pump | 5V mini submersible pump | Exact current draw must be measured or sourced from datasheet. |
| Moisture sensor | Capacitive analog module | Output must be 3.3V-safe. |
| Display | 0.96 inch SSD1306 I2C OLED | Confirm module pin order and voltage compatibility. |
| Encoder | EC11 rotary encoder with push | Confirm footprint variant and shaft/mechanical fit. |
| Error LED | Red LED plus resistor | Place where visible or route to UI board. |
| Connectors | JST-XH preferred | Confirm pitch, footprint, current, and cable orientation. |
| Passives | 0805 | Easier hand soldering. |
| Debug | Test pads / UART header | Include `+5V`, `+3V3`, `GND`, `EN`, `BOOT`, `TXD`, `RXD`, `PUMP_GATE`, `MOISTURE_ADC`, `I2C_SDA`, `I2C_SCL`. |

## Required BOM Categories

- USB-C receptacle and CC resistors.
- Input protection: optional polyfuse and/or TVS if selected.
- Bulk input capacitance for pump transients.
- Buck regulator, inductor, input capacitor, two output capacitors, and bootstrap capacitor.
- ESP32 module and support parts for EN/BOOT/programming.
- MOSFET driver parts: MOSFET, gate resistor, gate pulldown, flyback diode.
- Pump connector and pump wiring.
- Moisture sensor connector and ADC filter/protection parts.
- Reservoir switch connector and pull resistor.
- Optional flow sensor connector and level shifting/protection if needed.
- OLED/encoder/UI connector parts.
- LEDs and current-limiting resistors.
- Mounting holes and mechanical hardware.

## Footprints To Verify Before Ordering

- USB-C connector footprint against the exact part drawing.
- AP63203 package and recommended land pattern.
- Inductor pad size and height.
- ESP32-S3-WROOM-1 module footprint and antenna keepout.
- AO3400A SOT-23 pinout.
- SS34 package and polarity marking.
- JST connector pitch and pin 1 orientation.
- OLED header pin order.
- EC11 encoder mounting tabs and switch pins.
- Mounting hole diameter and enclosure hardware.

## Unknowns To Resolve

- Exact pump current draw and stall/startup behavior.
- Exact moisture sensor module pin order and output range.
- Exact OLED module pin order.
- Whether UI is on the main PCB or a cabled daughterboard.
- Exact USB-C receptacle part; Rev A schematic uses the 16-pin USB2 symbol with SBU pins no-connected.
- Exact enclosure mounting constraints.
- Target PCB manufacturer minimum trace/space, drill, and copper weight.
