# Total BOM For 5 Self-Watering Flower Pot V2 Units

Date checked: 2026-05-19

This is the minimum-buy order estimate for building 5 complete Rev A systems: 5 Board A PCBs, 5 Board B UI PCBs, and enough parts to assemble and operate 5 units. It includes current supplier minimum order quantities and pack sizes where they matter.

It excludes shipping, sales tax, tariffs, tools, solder/flux/wire, enclosure/base material, and optional flow sensors. Final cart totals can move noticeably because the order touches several suppliers.

## Total

| Scope | Estimated minimum checkout cost |
|---|---:|
| 5 complete systems, including 5 power supplies, bench USB-C cable, fallback USB-UART, and mounting hardware kits | $275.10 |
| Same order without power supplies and mounting hardware | $205.10 |
| Bare electronics/modules only, also excluding bench USB-C cable and USB-UART adapter | $191.10 |

Optional flow sensing is not included. If you add one flow sensor per unit, budget roughly another $45 plus any level-shifting/protection parts if the selected flow sensor outputs 5 V pulses.

## Major Cost Drivers

| Item | Buy quantity | Estimated line cost | Note |
|---|---:|---:|---|
| ESP32-S3-WROOM-1-N8 modules | 5 | $28.30 | DigiKey low-quantity price. |
| EC11 encoders | 5 | $19.30 | Mouser qty-5 price; verify shaft/panel clearance. |
| OLED modules | 5 | $19.00 | Makerfabs module; verify `GND,VCC,SCL,SDA` pin order. |
| Pump packs | 2 packs / 8 pumps | $23.96 | Gikfun sells the selected pump as a 4-pack. |
| Moisture sensors | 5 | $28.50 | DFRobot qty-5 price tier. |
| Reservoir float switches | 1 five-pack | $24.67 | Mechanical fit still depends on the reservoir/base design. |
| 5 V >=2 A power supplies | 5 | $50.00 | Needed if all 5 units will be deployed independently. |

## Minimum-Buy Notes

- The Board B right-angle 9-pin JST header source found has a minimum order of 10, so the BOM buys 10 even though only 5 are installed.
- The selected pump is sold as a 4-pack, so 5 systems require buying 2 packs and leaving 3 spare pumps.
- The JST crimp-contact line is the exact minimum count for pump, reservoir, moisture, and Board A to Board B UI cables: 125 contacts. Buy more in real life; hand crimping small contacts wastes parts.
- DNP capacitors are not included in the minimum build: Board A `C4` and Board B `C1-C3`.
- Optional flow hardware is not included. The Board A connector is populated, but the board works without a flow sensor.

## Lock Before Checkout

- Confirm `L1` physical fit against `Inductor_SMD:L_Bourns_SRN6045TA`.
- Confirm the exact USB-C suffix matches the KiCad USB4105 footprint.
- Confirm JST header orientation and pin 1 marks before ordering cable parts.
- Confirm OLED module pin order is `GND,VCC,SCL,SDA`.
- Confirm EC11 encoder shaft height and knob clearance against the UI panel.
- Confirm the selected reservoir switch physically fits the base/reservoir.
- Upload both Gerber zips to the PCB manufacturer viewer and inspect every layer before checkout.

## Files

- Detailed CSV: `fabrication/bom/total-bom-5-sets.csv`
- Board A fab zip: `fabrication/board_a/Self-Watering_Flower_Pot_Board_A_fab.zip`
- Board B fab zip: `fabrication/board_b/Self-Watering_UI_Board_B_fab.zip`
- Downloads copies:
  - `C:\Users\chesk\Downloads\Self-Watering_Flower_Pot_Board_A_fab.zip`
  - `C:\Users\chesk\Downloads\Self-Watering_UI_Board_B_fab.zip`

## Sources Checked

- JLCPCB prototype pricing page: https://jlcpcb.com/?from=getquote
- ESP32-S3-WROOM-1-N8: https://www.digikey.com/en/products/detail/espressif-systems/ESP32-S3-WROOM-1-N8/15200089
- AP63203WU-7: https://www.digikey.com/en/products/detail/diodes-incorporated/AP63203WU-7/9858426
- USB4105-GF-A: https://www.digikey.com/en/products/detail/gct/USB4105-GF-A/11198441
- ASPI-0628-3R9M-T1 inductor candidate: https://www.digikey.com/en/products/detail/abracon-llc/ASPI-0628-3R9M-T1/5043506
- AO3400A: https://www.lcsc.com/product-detail/MOSFET_Alpha-Omega-Semicon-AO3400A_C20917.html
- SS34-E3/57T: https://www.digikey.com/en/products/detail/vishay-general-semiconductor-diodes-division/SS34-E3-57T/1091554
- JST XH cable housings/contacts and EC11 encoder: Mouser/Newark/TrustedParts links in the CSV.
- Makerfabs OLED: https://www.makerfabs.com/0-96-inch-i2c-oled-128x64-blue.html
- Gikfun pump pack: https://gikfun.com/products/gikfun-dc-3v-5v-micro-submersible-mini-water-pump-pack-of-4pcs
- DFRobot SEN0193 moisture sensor: https://www.dfrobot.com/product-1385.html/
- PMD Way float-switch five-pack: https://pmdway.com/products/plastic-vertical-float-switches-five-pack
