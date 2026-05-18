# Pass 2 PCB Readiness

Pass 2 resolves the schematic-level blockers that prevented moving into PCB placement. The schematic is intended to be ready for `Update PCB from Schematic` after a visual review in KiCad GUI.

## Locked Rev-A Assumptions

- Main PCB uses a cabled UI connector. OLED, encoder, and front-panel LEDs are not placed directly on the main board in Rev A.
- USB-C is a 5V sink plus native USB2 data only. No USB-PD circuitry is included.
- Flow sensor support is optional-ready and defaults to a 3.3V-safe pulse signal.
- Moisture sensor is powered from `+3V3`; its analog output must stay within the ESP32 ADC input range.
- Pump is a 5V load switched by a low-side AO3400A MOSFET with a gate resistor, 100k pulldown, and SS34 flyback diode.

## Selected Footprints And Values

| Ref | Function | Pass 2 value / footprint | Notes |
|---|---|---|---|
| `J1` | USB-C input/native USB | `Connector_USB:USB_C_Receptacle_GCT_USB4105-xx-A_16P_TopMnt_Horizontal` | KiCad footprint pads match the USB-C USB2 symbol pad names. Confirm exact purchasable GCT USB4105 variant before ordering. |
| `U2` | 3.3V buck regulator | `AP63203WU-7`, `Package_TO_SOT_SMD:TSOT-23-6` | Fixed 3.3V AP63203 family part. Layout must follow datasheet. |
| `L1` | Buck inductor | `3.9uH shielded >=3A Isat`, `Inductor_SMD:L_Bourns_SRN6045TA` | Datasheet-aligned value for the AP63203 3.3V output; confirm final stocked part current, DCR, and height. |
| `C1` | Buck input capacitor | `10uF 10V X7R input`, `C_0805` | Place close to U2 VIN/GND. |
| `C2` | Buck output capacitor | `22uF 6.3V X7R output`, `C_0805` | Place close to inductor/output return path. |
| `C5` | Buck output capacitor | `22uF 6.3V X7R output`, `C_0805` | Second output capacitor added to match the AP63203 datasheet recommendation. |
| `C3` | Bootstrap capacitor | `100nF 10V BST`, `C_0805` | Keep loop short between BST and SW. |
| `C6` | Pump rail bulk capacitor | `100uF 10V pump bulk`, `CP_Radial_D6.3mm_P2.50mm` | Place near `J2` and observe polarity. |
| `R10` | Moisture ADC input resistor | `1k ADC series`, `R_0805` | Adds basic ADC input protection/isolation. |
| `C4` | Moisture ADC filter option | `100nF ADC filter DNP`, `C_0805` | Optional low-pass cap from ADC node to ground; leave DNP unless readings are noisy. |
| `TP1`-`TP5` | Bring-up test pads | `TestPoint_Pad_D1.5mm` | Nets are `+5V`, `GND`, `+3V3`, `PUMP_GATE`, and `MOISTURE_ADC`. |

## Connector Pinouts To Preserve

| Ref | Pin order |
|---|---|
| `J2` pump | `1=+5V`, `2=PUMP_LOW` switched by MOSFET |
| `J3` moisture | `1=+3V3`, `2=GND`, `3=MOISTURE_ADC` through `R10` |
| `J4` reservoir | `1=RESERVOIR_SW`, `2=GND`; switch closes to ground with `R8` pullup |
| `J5` flow optional | `1=+3V3`, `2=GND`, `3=FLOW_PULSE`; pulse must be 3.3V-safe |
| `J6` UI | `1=+3V3`, `2=GND`, `3=SDA`, `4=SCL`, `5=ENC_A`, `6=ENC_B`, `7=ENC_SW`, `8=ERROR_LED`, `9=STATUS_LED` |
| `J7` debug | `1=+3V3`, `2=GND`, `3=TXD0`, `4=RXD0`, `5=EN`, `6=IO0/BOOT` |

## PCB Placement Notes

- Place `J1` on a board edge before routing anything else.
- Place `U1` with antenna at a board edge and keep copper/mechanical interference out of the antenna area.
- Place `U2`, `C1`, `C2`, `C3`, `C5`, and `L1` as a tight buck-regulator cluster before routing other signals.
- Keep the pump current loop from `+5V -> J2 -> Q1 -> GND` short and away from `MOISTURE_ADC`.
- Place `C6` close to `J2` with a short return path into the main ground plane.
- Place `R10` and optional `C4` close to the ESP32 ADC pin side of the moisture signal.
- Route USB D+/D- intentionally as a short pair from `J1` to the ESP32 native USB pins.
- Put connector labels and pin-1 marks on silkscreen before first board order.

## Remaining Procurement Warnings

- Confirm the exact USB-C connector variant and print/check the footprint before ordering.
- Confirm the exact inductor part, height, saturation current, and availability.
- Confirm actual pump running and startup/stall current before choosing trace width.
- Confirm moisture sensor module pin order and analog output range.
- Confirm flow sensor voltage output before populating it.
