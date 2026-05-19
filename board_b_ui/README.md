# Board B UI Daughterboard

Board B connects to Board A J6 through J1 with the same pin order:

1. `+3V3`
2. `GND`
3. `SDA`
4. `SCL`
5. `ENC_A`
6. `ENC_B`
7. `ENC_SW`
8. `ERROR_LED`
9. `STATUS_LED`

The OLED header target is the common 4-pin SSD1306 I2C module order `GND`, `VCC/3V3`, `SCL`, `SDA`. Verify the actual module pin order before fabrication.

The PCB outline is 64 mm x 38 mm with M2.5 non-plated mounting holes. The front side is the user-facing panel with the OLED header/module outline, EC11 encoder footprint, error/status LEDs, and labels. The 9-pin JST-XH cable connector and the support R/C parts are on the back side.

Board B has explicit routed copper for every net plus front/back `GND` zones. Refill zones in KiCad with `B` after manual edits and rerun DRC before fabrication.

Current review/fabrication artifacts:

- Checks: `reports/erc-board-b-front-panel.txt`, `reports/drc-board-b-front-panel.txt`, and `reports/drc-board-b-front-panel-parity.txt`
- Routing review: `outputs/BoardB_UI_routing.svg` and `outputs/BoardB_UI_routing.pdf`
- Schematic PDF: `outputs/BoardB_UI_schematic.pdf`
- STEP: `mechanical/BoardB_UI.step`
- Gerbers and drills: `../fabrication/board_b/`

The KiCad footprint references an EC11 3D model under `Rotary_Encoder.3dshapes`. This KiCad install does not include that model, so the STEP export may omit the encoder body even though the footprint, holes, and courtyard are present.
