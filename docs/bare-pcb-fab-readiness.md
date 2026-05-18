# Bare-PCB Fab Readiness

Date: 2026-05-18

## Status

Board B is fab-ready for bare PCB ordering.

Board A is not fab-ready yet. ERC is clean and schematic parity is clean, but DRC still reports four real unconnected items. Do not order Board A until these are fixed in KiCad PCB editor and DRC is rerun clean.

## Board A Hold Items

Current reports:

- `reports/erc-fab-board-a.txt`: 0 violations.
- `reports/drc-fab-board-a-parity.txt`: 0 schematic parity issues.
- `reports/drc-fab-board-a.txt`: 4 unconnected items plus 2 silkscreen warnings.

Open routing/placement items:

- `R8` pad 1 to `Net-(J4-Pin_1)` reservoir switch signal.
- `C4` pad 1 to `Net-(TP5-Pin_1)` optional moisture ADC filter node.
- `R6` pad 2 to `Net-(J6-Pin_3)` SDA pullup branch.
- `U1` pad 18 to `J6` pin 5 / `Net-(J6-Pin_5)` encoder A.

Recommended Board A GUI fix:

1. Route `U1` pad 18 to `J6` pin 5 first. This is the hardest channel because the nearby `J6` pin 6 route and USB-related routes block simple script routes.
2. Move `R6` closer to `J6` pin 3 and `+3V3`, or place it on the back side if it clears courtyard/assembly constraints.
3. Move `R8` near `J4`/`J5` and connect pad 1 to reservoir signal and pad 2 to nearby `+3V3`.
4. Move `C4` beside `R10` or route its pad 1 to the existing moisture ADC series node.
5. Refill zones and rerun DRC with schematic parity.

Board A review artifacts:

- `outputs/BoardA_schematic_fab_review.pdf`
- `outputs/BoardA_routing_HOLD.svg`
- `outputs/BoardA_routing_HOLD.png`

## Board B Fab-Ready Outputs

Checks:

- `board_b_ui/reports/erc-board-b-fab.txt`: 0 violations.
- `board_b_ui/reports/drc-board-b-fab.txt`: 0 violations, 0 unconnected items.
- `board_b_ui/reports/drc-board-b-parity.txt`: 0 violations, 0 unconnected items, 0 schematic parity issues.

Bare-board output package:

- Gerbers: `fabrication/board_b/gerbers/`
- Drill files and drill maps: `fabrication/board_b/drill/`
- STEP: `board_b_ui/mechanical/BoardB_UI.step`
- Schematic PDF: `board_b_ui/outputs/BoardB_UI_schematic.pdf`
- Routing review: `board_b_ui/outputs/BoardB_UI_routing.svg`
- 3D render: `board_b_ui/outputs/BoardB_UI_top.png`

Known acceptable Board B warning during STEP export:

- KiCad could not find the EC11 encoder 3D model. The footprint, pads, and holes are present; the STEP may omit the encoder body.

## Order Checklist

Do not order both boards together yet.

Order Board B only if the physical OLED module pin order is confirmed as `GND, VCC, SCL, SDA`, and the right-angle 9-pin JST cable direction matches the enclosure plan.

Hold Board A until:

- `reports/drc-fab-board-a.txt` reports 0 errors and 0 unconnected items.
- `reports/drc-fab-board-a-parity.txt` reports 0 schematic parity issues.
- The `J6` pinout is visually rechecked against Board B `J1`:
  `1=+3V3`, `2=GND`, `3=SDA`, `4=SCL`, `5=ENC_A`, `6=ENC_B`, `7=ENC_SW`, `8=ERROR_LED`, `9=STATUS_LED`.
