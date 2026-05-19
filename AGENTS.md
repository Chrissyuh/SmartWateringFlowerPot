# KiCad Project Rules for Codex

This is a KiCad PCB project for the Self-Watering Flower Pot V2.

Rules:
- Do not blindly rewrite `.kicad_sch` or `.kicad_pcb` files.
- Before editing schematic or PCB files, explain the intended change first.
- Prefer using `kicad-cli`, Python scripts, reports, and small controlled edits.
- Run ERC after schematic changes when possible.
- Run DRC after PCB changes when possible.
- Put reports in `/reports`.
- Put generated manufacturing/export files in `/fabrication` or `/outputs`.
- Do not change footprints, net names, connector pinouts, power paths, board shape, mounting holes, or enclosure constraints unless explicitly asked.
- If a change is risky, create a script or patch instead of directly modifying the KiCad design file.
- KiCad GUI remains the source of truth for visual PCB edits.
- Treat `CODEX_HANDOVER.md` as the main project context file.
- Unless there is a good reason not to, push completed project changes to GitHub.
