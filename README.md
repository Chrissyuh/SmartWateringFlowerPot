# Smart Watering Flower Pot V2

KiCad project for a Smart Self-Watering Plant Pot Rev A prototype.

The project uses a hybrid workflow:
- KiCad GUI is the source of truth for visual schematic and PCB work.
- Codex creates documentation, checklists, reports, and export automation.
- Direct edits to `.kicad_sch` or `.kicad_pcb` require prior explanation and explicit approval.

## Current State

- KiCad project exists and is intentionally early/blank.
- `CODEX_HANDOVER.md` is the main design context.
- Initial ERC report passes because the schematic is blank.
- Initial DRC reports the expected blank-board issue: no `Edge.Cuts` outline exists yet.

## Useful Commands

```powershell
& 'C:\Users\chesk\AppData\Local\Programs\KiCad\9.0\bin\kicad-cli.exe' version
& 'C:\Users\chesk\AppData\Local\Programs\KiCad\9.0\bin\kicad-cli.exe' sch erc --format report --output 'reports\erc.txt' 'SmartWateringFlowerPot.kicad_sch'
& 'C:\Users\chesk\AppData\Local\Programs\KiCad\9.0\bin\kicad-cli.exe' pcb drc --format report --output 'reports\drc.txt' 'SmartWateringFlowerPot.kicad_pcb'
```

## Next Manual KiCad Step

Use `docs/schematic-planning.md`, `docs/pinout.md`, and `docs/bom-notes.md` to start placing schematic symbols in KiCad GUI. Do not start routing until the schematic has assigned footprints and ERC has been reviewed.
