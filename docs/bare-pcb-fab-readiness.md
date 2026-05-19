# Bare-PCB Fab Readiness

Date: 2026-05-19

## Status

Board A and Board B are both fab-ready for bare PCB manufacturer preview.

Board A now passes ERC, DRC, and schematic parity with 0 violations and 0 unconnected pads after the four open connections were fixed and copper zones were refilled. Board B remains clean after the front-panel placement cleanup and refreshed fabrication export.

One-large-order readiness status is tracked in `docs/one-large-order-readiness.md`. The current purchasing BOM is `fabrication/bom/order-readiness-purchasing-bom.csv`; it still needs a live stock/price refresh before parts checkout.

## Board A Fab-Ready Outputs

Current reports:

- `reports/erc-order-batch-board-a.txt`: 0 violations.
- `reports/drc-order-batch-board-a.txt`: 0 violations, 0 unconnected pads.
- `reports/drc-order-batch-board-a-parity.txt`: 0 violations, 0 unconnected pads, 0 schematic parity issues.

Fixed routing/placement items:

- `R8` pad 1 to `Net-(J4-Pin_1)` reservoir switch signal.
- `C4` pad 1 to `Net-(TP5-Pin_1)` optional moisture ADC filter node.
- `R6` pad 2 to `Net-(J6-Pin_3)` SDA pullup branch.
- `U1` pad 18 to `J6` pin 5 / `Net-(J6-Pin_5)` encoder A.

Board A bare-board output package:

- Gerbers: `fabrication/board_a/gerbers/`
- Drill files and drill maps: `fabrication/board_a/drill/`
- Fab zip: `fabrication/board_a/Self-Watering_Flower_Pot_Board_A_fab.zip`
- STEP: `mechanical/BoardA.step`
- Schematic PDF: `outputs/BoardA_schematic_order_batch.pdf`
- Routing review SVG/PDF: `outputs/BoardA_routing_order_batch.svg`, `outputs/BoardA_routing_order_batch.pdf`
- 3D review images: `outputs/BoardA_3d_top.png`, `outputs/BoardA_3d_bottom.png`

## Board B Fab-Ready Outputs

Checks:

- `board_b_ui/reports/erc-board-b-front-panel.txt`: 0 violations.
- `board_b_ui/reports/drc-board-b-front-panel.txt`: 0 violations, 0 unconnected items.
- `board_b_ui/reports/drc-board-b-front-panel-parity.txt`: 0 violations, 0 unconnected items, 0 schematic parity issues.
- `board_b_ui/reports/erc-one-large-order-board-b.txt`: 0 ERC messages, 0 errors, 0 warnings.
- `board_b_ui/reports/drc-one-large-order-board-b.txt`: 0 DRC violations, 0 unconnected items.
- `board_b_ui/reports/drc-one-large-order-board-b-parity.txt`: 0 DRC violations, 0 unconnected items.
- `board_b_ui/reports/erc-order-batch-board-b.txt`: 0 violations.
- `board_b_ui/reports/drc-order-batch-board-b.txt`: 0 violations, 0 unconnected items.
- `board_b_ui/reports/drc-order-batch-board-b-parity.txt`: 0 violations, 0 unconnected items, 0 schematic parity issues.

Bare-board output package:

- Gerbers: `fabrication/board_b/gerbers/`
- Drill files and drill maps: `fabrication/board_b/drill/`
- STEP: `board_b_ui/mechanical/BoardB_UI.step`
- Schematic PDF: `board_b_ui/outputs/BoardB_UI_schematic.pdf`
- Routing review: `board_b_ui/outputs/BoardB_UI_routing.svg`
- Routing review PDF: `board_b_ui/outputs/BoardB_UI_routing.pdf`
- Fab zip: `fabrication/board_b/Self-Watering_UI_Board_B_fab.zip`
- 3D review images: `board_b_ui/outputs/BoardB_UI_3d_top.png`, `board_b_ui/outputs/BoardB_UI_3d_bottom.png`

Known acceptable Board B warning during STEP export:

- KiCad could not find the EC11 encoder 3D model. The footprint, pads, and holes are present; the STEP may omit the encoder body.

## Order Checklist

Before ordering both boards together:

- Upload both final Gerber zip files to the PCB manufacturer's previewer.
- Inspect board outline, holes, copper, solder mask, silkscreen, and pin-1 marks for both boards.
- Visually recheck Board A `J6` against Board B `J1`:
  `1=+3V3`, `2=GND`, `3=SDA`, `4=SCL`, `5=ENC_A`, `6=ENC_B`, `7=ENC_SW`, `8=ERROR_LED`, `9=STATUS_LED`.
- Confirm Board B back-side right-angle 9-pin JST cable direction in 3D view against the enclosure/front-panel plan.
- Confirm the physical OLED module pin order is `GND, VCC, SCL, SDA`.
- Refresh the purchasing BOM live before checking out parts.
