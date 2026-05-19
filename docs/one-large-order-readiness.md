# One Large Order Readiness

Date: 2026-05-19

## Status

CAD package ready for manufacturer preview, with pre-order hardening notes added.

Board A and Board B now both pass ERC, DRC, and schematic-parity checks with 0 violations and 0 unconnected pads. Board A Gerbers, drill files, STEP, schematic PDF, routing review files, and 3D review images were regenerated after the four open connections were fixed and zones were refilled.

Do not check out the order until the Gerber zips are uploaded to the PCB manufacturer's viewer and visually inspected. The current purchasing BOM is still a readiness BOM: it locks practical part targets, but live supplier stock/prices and the mechanical warning items must be refreshed right before buying parts.

The hardening review did not find a CAD-clean blocker. It did identify prototype constraints that must be accepted before ordering Rev A: no USB input fuse/current limiter, no USB data ESD array, and no extra board-level ESP32 bulk capacitor directly beside U1. See `docs/pre-order-hardening-review.md`.

## Board A

Current checks:

- `reports/erc-order-batch-board-a.txt`: 0 violations.
- `reports/drc-order-batch-board-a.txt`: 0 violations, 0 unconnected pads.
- `reports/drc-order-batch-board-a-parity.txt`: 0 violations, 0 unconnected pads, 0 schematic parity issues.
- `reports/erc-hardening-board-a.txt`: 0 violations.
- `reports/drc-hardening-board-a.txt`: 0 violations, 0 unconnected pads.
- `reports/drc-hardening-board-a-parity.txt`: 0 violations, 0 unconnected pads, 0 schematic parity issues.

The four previous ordering blockers were fixed:

- `R8` pad 1 to `Net-(J4-Pin_1)` reservoir switch signal.
- `C4` pad 1 to `Net-(TP5-Pin_1)` optional moisture ADC filter/test node.
- `R6` pad 2 to `Net-(J6-Pin_3)` SDA pullup branch.
- `U1` pad 18 to `J6` pin 5 / `Net-(J6-Pin_5)` encoder A.

Board A package regenerated in this pass:

- Gerbers: `fabrication/board_a/gerbers/`
- Drill files and drill maps: `fabrication/board_a/drill/`
- Fab zip: `fabrication/board_a/SmartWateringFlowerPot_board_a_fab.zip`
- STEP: `mechanical/BoardA.step`
- Schematic PDF: `outputs/BoardA_schematic_order_batch.pdf`
- Routing review SVG/PDF: `outputs/BoardA_routing_order_batch.svg`, `outputs/BoardA_routing_order_batch.pdf`
- 3D review images: `outputs/BoardA_3d_top.png`, `outputs/BoardA_3d_bottom.png`

## Board B

Current checks:

- `board_b_ui/reports/erc-order-batch-board-b.txt`: 0 violations.
- `board_b_ui/reports/drc-order-batch-board-b.txt`: 0 violations, 0 unconnected pads.
- `board_b_ui/reports/drc-order-batch-board-b-parity.txt`: 0 violations, 0 unconnected pads, 0 schematic parity issues.
- `board_b_ui/reports/erc-hardening-board-b.txt`: 0 violations.
- `board_b_ui/reports/drc-hardening-board-b.txt`: 0 violations, 0 unconnected pads.
- `board_b_ui/reports/drc-hardening-board-b-parity.txt`: 0 violations, 0 unconnected pads, 0 schematic parity issues.

Board B package refreshed in this pass:

- Gerbers: `fabrication/board_b/gerbers/`
- Drill files and drill maps: `fabrication/board_b/drill/`
- Fab zip: `fabrication/board_b/BoardB_UI_fab.zip`
- STEP: `board_b_ui/mechanical/BoardB_UI.step`
- Schematic PDF: `board_b_ui/outputs/BoardB_UI_schematic.pdf`
- Routing review SVG/PDF: `board_b_ui/outputs/BoardB_UI_routing.svg`, `board_b_ui/outputs/BoardB_UI_routing.pdf`
- 3D review images: `board_b_ui/outputs/BoardB_UI_3d_top.png`, `board_b_ui/outputs/BoardB_UI_3d_bottom.png`

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
- Buy a dedicated 5 V, >=2 A USB adapter or USB power bank for pump tests. Do not run the pump from a laptop USB port.
- Buy a real USB-C data cable and a 3.3 V USB-UART adapter for fallback flashing through J7.

## Final Order Gate

Before ordering both boards and parts together:

1. Upload both final Gerber zip packages to the PCB manufacturer's previewer and inspect outlines, holes, copper, silkscreen, and solder mask.
2. Recheck Board A `J6` against Board B `J1`: `1=+3V3`, `2=GND`, `3=SDA`, `4=SCL`, `5=ENC_A`, `6=ENC_B`, `7=ENC_SW`, `8=ERROR_LED`, `9=STATUS_LED`.
3. In KiCad 3D viewer or the enclosure CAD, confirm Board B back-side cable direction, OLED fit, encoder clearance, LED visibility, and mounting holes.
4. Refresh the purchasing BOM prices and stock immediately before checkout.
5. Resolve the remaining BOM warning items: L1 footprint fit, JST stocked equivalents, OLED pin order, EC11 shaft/panel clearance, reservoir switch geometry, and optional flow-sensor level safety.
6. Explicitly accept the Rev A protection tradeoff documented in `docs/pre-order-hardening-review.md`, or stop and make a Rev A.1 ECO for USB fuse/ESD/local ESP32 bulk capacitance.
