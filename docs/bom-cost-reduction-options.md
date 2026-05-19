# BOM Cost Reduction Options

Date checked: 2026-05-19

Baseline: `docs/total-bom-5-sets.md` estimates a 5-system minimum-buy order at $275.10 before shipping, tax, tariffs, tools, and enclosure material.

This file lists cost reductions found in a live supplier search. Do not apply the risky substitutions directly to the purchasing BOM until the part is physically/electrically verified.

## Best Savings

| Change | Current line | Lower-cost option found | Estimated savings | Risk |
|---|---:|---:|---:|---|
| Buy lower-cost 5 V / 2 A adapters | $50.00 | xUmp 5 V / 2 A adapters at $3.59 each, 5 = $17.95 | $32.05 | Medium: mains adapter quality/safety matters. Prefer certified adapters or known-good adapters already on hand. |
| Use generic capacitive soil sensors | $28.50 | AliExpress-listed V1.2 sensor at about $0.85 each, 5 = $4.25 | $24.25 | High: generic V1.2 sensors vary; confirm 3.3 V operation, output range, coating, and calibration. |
| Use a cheaper float-switch pack | $24.67 | AliExpress listing around $4.29 for the 1/5-piece variation | about $20.38 | High: listing variation, mechanical dimensions, wire length, and plastic compatibility must be verified. |
| Source EC11 encoder from LCSC | $19.30 | LCSC ALPSALPINE EC11E1820402 at about $1.616 each, 5 = $8.08 | $11.22 | Medium: verify footprint, shaft height, detents, and push-switch pins against Board B before buying. |
| Source SS34 from LCSC | $5.10 | LCSC BORN SS34, minimum 20 at about $0.0537 each, 20 = $1.07 | $4.03 | Low/medium: same SMA/DO-214AC class, but manufacturer changes. |
| Source ESP32-S3 module from LCSC | $28.30 | LCSC ESP32-S3-WROOM-1-N8 at about $4.773 each, 5 = $23.87 | $4.43 | Low/medium: same Espressif module target, but supplier/shipping changes. |
| Source AP63203 from LCSC | $4.00 | LCSC AP63203WU-7 at about $0.4705 each, 5 = $2.35 | $1.65 | Low/medium: same manufacturer part; savings only matter if already ordering from LCSC. |

If every option above works, the paper total drops from $275.10 to about $177.10. That is the aggressive-cost case and includes high-risk module substitutions.

Conservative supplier-only swaps, keeping DFRobot moisture sensors and PMD Way reservoir switches and not changing the power supplies, lower the total only to about $253.78. With the lower-cost adapters included, it becomes about $221.73.

## Recommended Path

1. Keep the exact PCB design.
2. Do not change the ESP32, AP63203, SS34, or EC11 footprints unless the alternate part drawings are checked against the current footprints.
3. Use LCSC for the AP63203, SS34, ESP32 module, and EC11 only if shipping is already justified by the order. Otherwise, DigiKey/Mouser consolidation may be cheaper at checkout.
4. For the moisture sensor and float switch, buy one cheap sample first if time allows. These are the biggest savings, but they are also the easiest place to lose reliability.
5. Do not cheap out blindly on mains power. Using adapters you already own is fine for prototypes; buying unknown wall adapters for deployed units is a safety/reliability tradeoff.

## Not Worth Changing Right Now

- PCB fabrication: Board A and Board B are already cheap. Combining them into a breakaway panel is not worth the added layout/manufacturing risk for this first order.
- OLED: Makerfabs is already $3.80 each at qty 5 and gives a clear module target. Cheaper marketplace OLEDs exist, but pin order is the main risk.
- Pump: cheaper international pump listings exist, but the selected Gikfun pump pack is already reasonable and has a known 3 V to 5 V target. Pump current/noise is a bring-up risk, so changing it for small savings is not the first move.
- Passives: the total line cost is too small to matter unless combined with a broader LCSC/Mouser consolidation.

## Sources Checked

- xUmp 5 V / 2 A adapter: https://www.xump.com/science/fast-usb-charger-power-supply-5v-2a.cfm
- AliExpress/PriceArchive generic capacitive sensor: https://ms.pricearchive.org/aliexpress.com/item/1005008718213399
- AliExpress/PriceArchive float-switch listing: https://no.pricearchive.org/aliexpress.com/item/1005009114140390
- LCSC ESP32-S3-WROOM-1-N8: https://www.lcsc.com/product-image/C2913198.html
- LCSC AP63203WU-7: https://www.lcsc.com/product-detail/C780769.html
- LCSC BORN SS34: https://www.lcsc.com/product-detail/C266553.html
- LCSC ALPSALPINE EC11E1820402: https://www.lcsc.com/product-detail/C361167.html
- Makerfabs OLED baseline: https://www.makerfabs.com/0-96-inch-i2c-oled-128x64-blue.html
- Thingbits low-cost 3 V to 5 V pump reference: https://www.thingbits.in/products/mini-vertical-submersible-pump-3v-to-5v-dc
- DFRobot SEN0193 baseline: https://www.dfrobot.com/product-1385.html/
- PMD Way float-switch baseline: https://pmdway.com/products/plastic-vertical-float-switches-five-pack
