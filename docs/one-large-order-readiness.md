# One Large Order Readiness

Date: 2026-05-19

## Status

Do not order yet.

Board B is still electrically and fabrication clean. Board A is still the blocker for a combined order because the current PCB has four real unconnected items. I did not export Board A Gerbers, drill files, or STEP in this pass because that would create an orderable-looking package from a board that still fails DRC.

The current purchasing BOM is a readiness BOM, not a buy-now BOM. It locks practical part targets and supplier checks, but the quantities and supplier carts should be refreshed after Board A is DRC-clean.

## Board A

Current checks:

- `reports/erc-one-large-order-board-a.txt`: 0 ERC messages, 0 errors, 0 warnings.
- `reports/drc-one-large-order-board-a.txt`: 2 silkscreen warnings and 4 unconnected pads.
- `reports/drc-one-large-order-board-a-parity.txt`: same DRC hold items; no separate schematic-parity mismatch was reported.

The four ordering blockers are:

- `R8` pad 1 to `Net-(J4-Pin_1)` reservoir switch signal.
- `C4` pad 1 to `Net-(TP5-Pin_1)` optional moisture ADC filter/test node.
- `R6` pad 2 to `Net-(J6-Pin_3)` SDA pullup branch.
- `U1` pad 18 to `J6` pin 5 / `Net-(J6-Pin_5)` encoder A.

I tried a scripted Board A route cleanup and reverted it because the lower connector area produced real DRC shorts and clearance issues. These four fixes should be done in KiCad PCB editor with visual routing, then zones should be refilled and DRC rerun.

Board A safe review outputs from this pass:

- `outputs/BoardA_schematic_order_hold.pdf`
- `outputs/BoardA_routing_ORDER_HOLD.svg`

No Board A bare-PCB fab package was regenerated in this pass.

## Board B

Current checks:

- `board_b_ui/reports/erc-one-large-order-board-b.txt`: 0 ERC messages, 0 errors, 0 warnings.
- `board_b_ui/reports/drc-one-large-order-board-b.txt`: 0 DRC violations, 0 unconnected pads, 0 footprint errors.
- `board_b_ui/reports/drc-one-large-order-board-b-parity.txt`: 0 DRC violations, 0 unconnected pads, 0 footprint errors.

Board B package regenerated in this pass:

- Gerbers: `fabrication/board_b/gerbers/`
- Drill files and drill maps: `fabrication/board_b/drill/`
- STEP: `board_b_ui/mechanical/BoardB_UI.step`
- Schematic PDF: `board_b_ui/outputs/BoardB_UI_schematic.pdf`
- Routing review SVG/PDF: `board_b_ui/outputs/BoardB_UI_routing.svg`, `board_b_ui/outputs/BoardB_UI_routing.pdf`

Known acceptable Board B issue:

- KiCad still cannot load the EC11 encoder 3D model during STEP export. The pads and holes are present; the exported STEP may omit the encoder body. Check actual knob/shaft clearance against the enclosure manually.

## BOM Files

Generated KiCad BOM exports:

- Board A: `fabrication/bom/board_a-kicad-bom.csv`
- Board B: `fabrication/bom/board_b-kicad-bom.csv`

Order-readiness purchasing BOM:

- `fabrication/bom/order-readiness-purchasing-bom.csv`

Live supplier findings captured in the purchasing BOM:

- Critical ICs and USB-C connector are available from DigiKey in low quantities.
- Board B right-angle 9-pin JST XH header is available from distributor listings such as Newark/Mouser in low quantities.
- Some Board A JST XH natural top-entry headers had weak distributor stock during this check; use stocked exact-footprint equivalents, wait for incoming stock, or recheck equivalent JST XH color/key variants before buying.
- The L1 candidate is electrically reasonable, but its body is 6.0 mm x 6.0 mm and must be checked against the current `Inductor_SMD:L_Bourns_SRN6045TA` footprint before ordering.
- The OLED module is not locked until the actual module pin order is visually confirmed as `GND, VCC, SCL, SDA`.
- Do not buy a 12 V pump for the current board. Board A switches USB 5 V, so use a small 3 V to 5 V or 5 V pump only.

## Final Order Gate

Before ordering both boards and parts together:

1. Fix the four Board A opens in KiCad PCB editor.
2. Refill all copper zones.
3. Run Board A ERC, DRC, and schematic parity clean.
4. Regenerate Board A Gerbers, drills, STEP, schematic PDF, and routing review outputs only after Board A DRC reports 0 unconnected pads.
5. Recheck Board A `J6` against Board B `J1`: `1=+3V3`, `2=GND`, `3=SDA`, `4=SCL`, `5=ENC_A`, `6=ENC_B`, `7=ENC_SW`, `8=ERROR_LED`, `9=STATUS_LED`.
6. Upload both final Gerber zip packages to the PCB manufacturer's previewer and inspect outlines, holes, copper, silkscreen, and solder mask.
7. Refresh the purchasing BOM prices and stock immediately before checkout.
