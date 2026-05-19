# Self-Watering Flower Pot V2

KiCad project for a Self-Watering Flower Pot V2 Rev A prototype.

The project uses a hybrid workflow:
- KiCad GUI is the source of truth for visual schematic and PCB work.
- Codex creates documentation, checklists, reports, and export automation.
- Direct edits to `.kicad_sch` or `.kicad_pcb` require prior explanation and explicit approval.

## Current State

- Current branch `codex/cost-down-board-a-only` is a cheaper Board-A-only first build.
- Board A cost-down PCB is clean: ERC 0, DRC 0, unconnected 0, schematic parity 0.
- Board B UI remains in the repo as a deferred optional board, but it is not part of this cost-down order.
- `CODEX_HANDOVER.md` is the main design context.
- The cost-down five-system minimum-buy BOM estimate is in `fabrication/bom/cost-down-5-sets.csv`.
- Final checkout is still gated by PCB manufacturer previews, live BOM stock/price refresh, and mechanical fit checks.

## Useful Commands

```powershell
& 'C:\Users\chesk\AppData\Local\Programs\KiCad\9.0\bin\kicad-cli.exe' version
& 'C:\Users\chesk\AppData\Local\Programs\KiCad\9.0\bin\kicad-cli.exe' sch erc --format report --output 'reports\erc.txt' 'SmartWateringFlowerPot.kicad_sch'
& 'C:\Users\chesk\AppData\Local\Programs\KiCad\9.0\bin\kicad-cli.exe' pcb drc --format report --output 'reports\drc.txt' 'SmartWateringFlowerPot.kicad_pcb'
```

## Next Manual Order Step

Upload only the Board A cost-down fab zip to the PCB manufacturer's previewer, inspect every layer, and verify the live purchasing BOM before checkout.
