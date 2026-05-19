# Self-Watering Flower Pot V2

KiCad project for a Self-Watering Flower Pot V2 Rev A prototype.

The project uses a hybrid workflow:
- KiCad GUI is the source of truth for visual schematic and PCB work.
- Codex creates documentation, checklists, reports, and export automation.
- Direct edits to `.kicad_sch` or `.kicad_pcb` require prior explanation and explicit approval.

## Current State

- Board A main-controller PCB and Board B UI PCB are ready for manufacturer preview.
- `CODEX_HANDOVER.md` is the main design context.
- Current ERC, DRC, and schematic-parity reports are clean for both boards.
- The current 5-system minimum-buy BOM estimate is in `docs/total-bom-5-sets.md`.
- Final checkout is still gated by PCB manufacturer previews, live BOM stock/price refresh, and mechanical fit checks.

## Useful Commands

```powershell
& 'C:\Users\chesk\AppData\Local\Programs\KiCad\9.0\bin\kicad-cli.exe' version
& 'C:\Users\chesk\AppData\Local\Programs\KiCad\9.0\bin\kicad-cli.exe' sch erc --format report --output 'reports\erc.txt' 'SmartWateringFlowerPot.kicad_sch'
& 'C:\Users\chesk\AppData\Local\Programs\KiCad\9.0\bin\kicad-cli.exe' pcb drc --format report --output 'reports\drc.txt' 'SmartWateringFlowerPot.kicad_pcb'
```

## Next Manual Order Step

Upload the Board A and Board B fab zips to the PCB manufacturer's previewer, inspect every layer, and verify the live purchasing BOM before checkout.
