from __future__ import annotations

import re
import uuid
from pathlib import Path

import pcbnew


ROOT = Path(__file__).resolve().parents[1]
SCH = ROOT / "SmartWateringFlowerPot.kicad_sch"
PCB = ROOT / "SmartWateringFlowerPot.kicad_pcb"
KICAD_FOOTPRINTS = Path(r"C:\Users\chesk\AppData\Local\Programs\KiCad\9.0\share\kicad\footprints")
SHEET_UUID = "f5104368-02d7-408f-bed6-2d0faa538216"


def u() -> str:
    return str(uuid.uuid4())


def instance_path(ref: str) -> str:
    return (
        '\t\t(instances\n'
        '\t\t\t(project ""\n'
        f'\t\t\t\t(path "/{SHEET_UUID}"\n'
        f'\t\t\t\t\t(reference "{ref}")\n'
        "\t\t\t\t\t(unit 1)\n"
        "\t\t\t\t)\n"
        "\t\t\t)\n"
        "\t\t)\n"
    )


def symbol_r(ref: str, value: str, x: float, y: float, sym_uuid: str) -> str:
    return (
        "\t(symbol\n"
        '\t\t(lib_id "Device:R")\n'
        f"\t\t(at {x:.2f} {y:.2f} 0)\n"
        "\t\t(unit 1)\n"
        "\t\t(exclude_from_sim no)\n"
        "\t\t(in_bom yes)\n"
        "\t\t(on_board yes)\n"
        "\t\t(dnp no)\n"
        f'\t\t(uuid "{sym_uuid}")\n'
        f'\t\t(property "Reference" "{ref}"\n'
        f"\t\t\t(at {x:.2f} {y - 7.62:.2f} 0)\n"
        '\t\t\t(effects (font (size 1.27 1.27)))\n'
        "\t\t)\n"
        f'\t\t(property "Value" "{value}"\n'
        f"\t\t\t(at {x:.2f} {y - 5.08:.2f} 0)\n"
        '\t\t\t(effects (font (size 1.27 1.27)))\n'
        "\t\t)\n"
        '\t\t(property "Footprint" "Resistor_SMD:R_0805_2012Metric"\n'
        f"\t\t\t(at {x:.2f} {y:.2f} 0)\n"
        '\t\t\t(effects (font (size 1.27 1.27)) (hide yes))\n'
        "\t\t)\n"
        '\t\t(property "Datasheet" "~"\n'
        f"\t\t\t(at {x:.2f} {y:.2f} 0)\n"
        '\t\t\t(effects (font (size 1.27 1.27)) (hide yes))\n'
        "\t\t)\n"
        f'\t\t(pin "1"\n\t\t\t(uuid "{u()}")\n\t\t)\n'
        f'\t\t(pin "2"\n\t\t\t(uuid "{u()}")\n\t\t)\n'
        f"{instance_path(ref)}"
        "\t)\n"
    )


def symbol_led(ref: str, value: str, x: float, y: float, sym_uuid: str) -> str:
    return (
        "\t(symbol\n"
        '\t\t(lib_id "Device:LED")\n'
        f"\t\t(at {x:.2f} {y:.2f} 0)\n"
        "\t\t(unit 1)\n"
        "\t\t(exclude_from_sim no)\n"
        "\t\t(in_bom yes)\n"
        "\t\t(on_board yes)\n"
        "\t\t(dnp no)\n"
        f'\t\t(uuid "{sym_uuid}")\n'
        f'\t\t(property "Reference" "{ref}"\n'
        f"\t\t\t(at {x:.2f} {y - 7.62:.2f} 0)\n"
        '\t\t\t(effects (font (size 1.27 1.27)))\n'
        "\t\t)\n"
        f'\t\t(property "Value" "{value}"\n'
        f"\t\t\t(at {x:.2f} {y - 5.08:.2f} 0)\n"
        '\t\t\t(effects (font (size 1.27 1.27)))\n'
        "\t\t)\n"
        '\t\t(property "Footprint" "LED_SMD:LED_0805_2012Metric"\n'
        f"\t\t\t(at {x:.2f} {y:.2f} 0)\n"
        '\t\t\t(effects (font (size 1.27 1.27)) (hide yes))\n'
        "\t\t)\n"
        '\t\t(property "Datasheet" "~"\n'
        f"\t\t\t(at {x:.2f} {y:.2f} 0)\n"
        '\t\t\t(effects (font (size 1.27 1.27)) (hide yes))\n'
        "\t\t)\n"
        f'\t\t(pin "1"\n\t\t\t(uuid "{u()}")\n\t\t)\n'
        f'\t\t(pin "2"\n\t\t\t(uuid "{u()}")\n\t\t)\n'
        f"{instance_path(ref)}"
        "\t)\n"
    )


def symbol_tp(ref: str, value: str, x: float, y: float, sym_uuid: str) -> str:
    return (
        "\t(symbol\n"
        '\t\t(lib_id "Connector_Generic:Conn_01x01")\n'
        f"\t\t(at {x:.2f} {y:.2f} 0)\n"
        "\t\t(unit 1)\n"
        "\t\t(exclude_from_sim no)\n"
        "\t\t(in_bom no)\n"
        "\t\t(on_board yes)\n"
        "\t\t(dnp no)\n"
        f'\t\t(uuid "{sym_uuid}")\n'
        f'\t\t(property "Reference" "{ref}"\n'
        f"\t\t\t(at {x:.2f} {y - 3.81:.2f} 0)\n"
        '\t\t\t(effects (font (size 1.27 1.27)))\n'
        "\t\t)\n"
        f'\t\t(property "Value" "{value}"\n'
        f"\t\t\t(at {x:.2f} {y + 3.81:.2f} 0)\n"
        '\t\t\t(effects (font (size 1.27 1.27)))\n'
        "\t\t)\n"
        '\t\t(property "Footprint" "TestPoint:TestPoint_Pad_D1.5mm"\n'
        f"\t\t\t(at {x:.2f} {y + 6.35:.2f} 0)\n"
        '\t\t\t(effects (font (size 1.27 1.27)) (hide yes))\n'
        "\t\t)\n"
        '\t\t(property "Datasheet" "~"\n'
        f"\t\t\t(at {x:.2f} {y + 8.89:.2f} 0)\n"
        '\t\t\t(effects (font (size 1.27 1.27)) (hide yes))\n'
        "\t\t)\n"
        f'\t\t(pin "1"\n\t\t\t(uuid "{u()}")\n\t\t)\n'
        f"{instance_path(ref)}"
        "\t)\n"
    )


def label(name: str, x: float, y: float, angle: int = 0) -> str:
    justify = "left bottom" if angle == 0 else "right bottom"
    return (
        f'\t(label "{name}"\n'
        f"\t\t(at {x:.2f} {y:.2f} {angle})\n"
        "\t\t(effects\n"
        "\t\t\t(font\n"
        "\t\t\t\t(size 1.27 1.27)\n"
        "\t\t\t)\n"
        f"\t\t\t(justify {justify})\n"
        "\t\t)\n"
        f'\t\t(uuid "{u()}")\n'
        "\t)\n"
    )


def wire(x1: float, y1: float, x2: float, y2: float) -> str:
    return (
        "\t(wire\n"
        "\t\t(pts\n"
        f"\t\t\t(xy {x1:.2f} {y1:.2f}) (xy {x2:.2f} {y2:.2f})\n"
        "\t\t)\n"
        "\t\t(stroke\n"
        "\t\t\t(width 0)\n"
        "\t\t\t(type default)\n"
        "\t\t)\n"
        f'\t\t(uuid "{u()}")\n'
        "\t)\n"
    )


def find_instance_symbols(text: str) -> list[tuple[int, int, str]]:
    starts = [m.start() for m in re.finditer(r"\n\t\(symbol\n\t\t\(lib_id ", text)]
    blocks = []
    for start in starts:
        i = start + 1
        depth = 0
        in_str = False
        esc = False
        for j, ch in enumerate(text[i:], i):
            if in_str:
                if esc:
                    esc = False
                elif ch == "\\":
                    esc = True
                elif ch == '"':
                    in_str = False
            else:
                if ch == '"':
                    in_str = True
                elif ch == "(":
                    depth += 1
                elif ch == ")":
                    depth -= 1
                    if depth == 0:
                        block = text[i : j + 1]
                        ref_match = re.search(r'\(property "Reference" "([^"]+)"', block)
                        if ref_match:
                            blocks.append((i, j + 1, ref_match.group(1)))
                        break
    return blocks


def set_schematic_symbol(block: str, *, in_bom: str, on_board: str, dnp: str, value: str | None = None) -> str:
    block = re.sub(r"\(in_bom (yes|no)\)", f"(in_bom {in_bom})", block, count=1)
    block = re.sub(r"\(on_board (yes|no)\)", f"(on_board {on_board})", block, count=1)
    block = re.sub(r"\(dnp (yes|no)\)", f"(dnp {dnp})", block, count=1)
    if value is not None:
        block = re.sub(r'(\(property "Value" ")[^"]+(")', rf"\g<1>{value}\2", block, count=1)
    return block


def update_schematic() -> dict[str, str]:
    text = SCH.read_text(encoding="utf-8")
    changed = {}
    updates = {
        "J4": dict(in_bom="no", on_board="no", dnp="yes", value="RESERVOIR_SW optional pads only"),
        "J5": dict(in_bom="no", on_board="no", dnp="yes", value="FLOW optional pads only"),
        "J6": dict(in_bom="no", on_board="no", dnp="yes", value="UI connector removed cost-down"),
        "R6": dict(in_bom="no", on_board="no", dnp="yes", value="4.7k SDA DNP no local UI"),
        "R7": dict(in_bom="no", on_board="no", dnp="yes", value="4.7k SCL DNP no local UI"),
        "R8": dict(in_bom="no", on_board="no", dnp="yes", value="10k reservoir pullup DNP use internal pullup"),
    }
    blocks = find_instance_symbols(text)
    for start, end, ref in reversed(blocks):
        if ref in updates:
            old = text[start:end]
            new = set_schematic_symbol(old, **updates[ref])
            text = text[:start] + new + text[end:]
            changed[ref] = "dnp_no_board"

    additions = []
    # Name the reused GPIO nets and optional pad nets.
    additions += [
        label("ERROR_LED", 142.24, 111.76, 0),
        label("STATUS_LED", 142.24, 114.30, 0),
        label("RESERVOIR_SW", 130.81, 106.68, 0),
        label("FLOW_PULSE", 130.81, 109.22, 0),
    ]

    ids = {ref: u() for ref in ("R11", "D2", "R12", "D3", "TP6", "TP7", "TP8", "TP9", "TP10")}
    additions += [
        symbol_r("R11", "330R error LED", 175.26, 124.46, ids["R11"]),
        wire(175.26, 120.65, 170.18, 120.65),
        label("ERROR_LED", 170.18, 120.65, 180),
        wire(175.26, 128.27, 170.18, 128.27),
        label("ERR_LED_A", 170.18, 128.27, 180),
        symbol_led("D2", "ERROR red", 195.58, 128.27, ids["D2"]),
        wire(191.77, 128.27, 186.69, 128.27),
        label("GND", 186.69, 128.27, 180),
        wire(199.39, 128.27, 204.47, 128.27),
        label("ERR_LED_A", 204.47, 128.27, 0),
        symbol_r("R12", "330R status LED", 175.26, 139.70, ids["R12"]),
        wire(175.26, 135.89, 170.18, 135.89),
        label("STATUS_LED", 170.18, 135.89, 180),
        wire(175.26, 143.51, 170.18, 143.51),
        label("STAT_LED_A", 170.18, 143.51, 180),
        symbol_led("D3", "STATUS green", 195.58, 143.51, ids["D3"]),
        wire(191.77, 143.51, 186.69, 143.51),
        label("GND", 186.69, 143.51, 180),
        wire(199.39, 143.51, 204.47, 143.51),
        label("STAT_LED_A", 204.47, 143.51, 0),
        symbol_tp("TP6", "RESERVOIR_SW optional pad", 175.26, 154.94, ids["TP6"]),
        wire(170.18, 154.94, 165.10, 154.94),
        label("RESERVOIR_SW", 165.10, 154.94, 180),
        symbol_tp("TP7", "OPT_GND pad", 175.26, 162.56, ids["TP7"]),
        wire(170.18, 162.56, 165.10, 162.56),
        label("GND", 165.10, 162.56, 180),
        symbol_tp("TP8", "OPT_3V3 pad", 205.74, 154.94, ids["TP8"]),
        wire(200.66, 154.94, 195.58, 154.94),
        label("+3V3", 195.58, 154.94, 180),
        symbol_tp("TP9", "OPT_GND pad", 205.74, 162.56, ids["TP9"]),
        wire(200.66, 162.56, 195.58, 162.56),
        label("GND", 195.58, 162.56, 180),
        symbol_tp("TP10", "FLOW_PULSE optional pad", 205.74, 170.18, ids["TP10"]),
        wire(200.66, 170.18, 195.58, 170.18),
        label("FLOW_PULSE", 195.58, 170.18, 180),
        (
            '\t(text "Cost-down Rev A: no local OLED/encoder UI. Reservoir and flow are optional pads; firmware defaults disabled."\n'
            "\t\t(at 121.92 167.64 0)\n"
            "\t\t(effects\n"
            "\t\t\t(font\n"
            "\t\t\t\t(size 1.27 1.27)\n"
            "\t\t\t)\n"
            "\t\t)\n"
            f'\t\t(uuid "{u()}")\n'
            "\t)\n"
        ),
    ]
    insert_at = text.index("\n\t(no_connect")
    text = text[:insert_at] + "\n" + "".join(additions) + text[insert_at:]
    SCH.write_text(text, encoding="utf-8", newline="\n")
    changed.update(ids)
    return changed


def get_net(board: pcbnew.BOARD, name: str) -> pcbnew.NETINFO_ITEM:
    nets = board.GetNetsByName()
    if name in nets:
        return nets[name]
    net = pcbnew.NETINFO_ITEM(board, name)
    board.Add(net)
    return net


def load_fp(lib: str, name: str) -> pcbnew.FOOTPRINT:
    lib_path = str(KICAD_FOOTPRINTS / f"{lib}.pretty")
    fp = pcbnew.FootprintLoad(lib_path, name)
    if fp is None:
        raise RuntimeError(f"Could not load footprint {lib}:{name}")
    return fp


def configure_fp(
    fp: pcbnew.FOOTPRINT,
    ref: str,
    value: str,
    lib: str,
    name: str,
    path_uuid: str,
    x: float,
    y: float,
    rot: float = 0,
) -> None:
    fp.SetFPID(pcbnew.LIB_ID(lib, name))
    fp.SetReference(ref)
    fp.SetValue(value)
    fp.SetPosition(pcbnew.VECTOR2I_MM(x, y))
    fp.SetOrientationDegrees(rot)
    fp.SetPath(pcbnew.KIID_PATH(f"/{path_uuid}"))
    fp.SetSheetfile("SmartWateringFlowerPot.kicad_sch")
    fp.SetSheetname("/")
    fp.Reference().SetVisible(False)
    fp.Value().SetVisible(False)


def remove_footprints(board: pcbnew.BOARD, refs: set[str]) -> None:
    for fp in list(board.GetFootprints()):
        if fp.GetReference() in refs:
            board.Delete(fp)


def remove_tracks_for_nets(board: pcbnew.BOARD, names: set[str]) -> None:
    for item in list(board.GetTracks()):
        if item.GetNetname() in names:
            board.Delete(item)


def set_pad_net(board: pcbnew.BOARD, ref: str, pad_no: str, net_name: str) -> None:
    fp = board.FindFootprintByReference(ref)
    if fp is None:
        raise RuntimeError(f"Missing footprint {ref}")
    pad = fp.FindPadByNumber(pad_no)
    if pad is None:
        raise RuntimeError(f"Missing pad {ref}.{pad_no}")
    pad.SetNet(get_net(board, net_name))


def touches_any(item: pcbnew.BOARD_CONNECTED_ITEM, points: set[tuple[float, float]], tol: float = 0.002) -> bool:
    if isinstance(item, pcbnew.PCB_VIA):
        p = item.GetPosition()
        x, y = p.x / 1e6, p.y / 1e6
        return any(abs(x - px) <= tol and abs(y - py) <= tol for px, py in points)
    if not hasattr(item, "GetStart"):
        return False
    start = item.GetStart()
    end = item.GetEnd()
    ends = [(start.x / 1e6, start.y / 1e6), (end.x / 1e6, end.y / 1e6)]
    return any(abs(x - px) <= tol and abs(y - py) <= tol for x, y in ends for px, py in points)


def remove_stale_cost_down_stubs(board: pcbnew.BOARD) -> None:
    # Delete only short branches left behind by removed UI/reservoir pullups and connectors.
    # Keep the main 3V3 rail and the routed reservoir/flow optional pad signal paths.
    stale_3v3_points = {
        (58.5090, 103.0010),
        (59.6800, 101.8300),
        (59.6800, 93.0000),
        (63.0875, 93.0000),
        (65.5040, 108.5440),
        (66.3175, 107.7300),
        (86.6300, 115.2000),
        (91.6875, 115.2000),
    }
    doomed = []
    for item in board.GetTracks():
        if item.GetNetname() == "+3V3" and touches_any(item, stale_3v3_points):
            doomed.append(item)
            continue
        if not hasattr(item, "GetStart"):
            continue
        pts = {
            (round(item.GetStart().x / 1e6, 4), round(item.GetStart().y / 1e6, 4)),
            (round(item.GetEnd().x / 1e6, 4), round(item.GetEnd().y / 1e6, 4)),
        }
        if item.GetNetname() == "Net-(J4-Pin_1)" and (
            (93.5125, 115.2) in pts or (98.58, 115.2) in pts
        ):
            doomed.append(item)
    for item in doomed:
        board.Delete(item)


def rename_net_items(board: pcbnew.BOARD, old: str, new: str) -> None:
    net = get_net(board, new)
    for fp in board.GetFootprints():
        for pad in fp.Pads():
            if pad.GetNetname() == old:
                pad.SetNet(net)
    for item in board.GetTracks():
        if item.GetNetname() == old:
            item.SetNet(net)
    for zone in board.Zones():
        if zone.GetNetname() == old:
            zone.SetNet(net)


def add_track(board: pcbnew.BOARD, net_name: str, x1: float, y1: float, x2: float, y2: float, width: float = 0.25, layer: int = pcbnew.F_Cu) -> None:
    track = pcbnew.PCB_TRACK(board)
    track.SetStart(pcbnew.VECTOR2I_MM(x1, y1))
    track.SetEnd(pcbnew.VECTOR2I_MM(x2, y2))
    track.SetWidth(pcbnew.FromMM(width))
    track.SetLayer(layer)
    track.SetNet(get_net(board, net_name))
    board.Add(track)


def add_via(board: pcbnew.BOARD, net_name: str, x: float, y: float, diameter: float = 0.6, drill: float = 0.3) -> None:
    via = pcbnew.PCB_VIA(board)
    via.SetPosition(pcbnew.VECTOR2I_MM(x, y))
    via.SetWidth(pcbnew.FromMM(diameter))
    via.SetDrill(pcbnew.FromMM(drill))
    via.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
    via.SetNet(get_net(board, net_name))
    board.Add(via)


def add_text(board: pcbnew.BOARD, text: str, x: float, y: float, size: float = 0.9) -> None:
    t = pcbnew.PCB_TEXT(board)
    t.SetText(text)
    t.SetPosition(pcbnew.VECTOR2I_MM(x, y))
    t.SetLayer(pcbnew.F_SilkS)
    t.SetTextSize(pcbnew.VECTOR2I_MM(size, size))
    t.SetTextThickness(pcbnew.FromMM(0.12))
    board.Add(t)


def update_pcb(ids: dict[str, str]) -> None:
    board = pcbnew.LoadBoard(str(PCB))
    remove_footprints(board, {"J4", "J5", "J6", "R6", "R7", "R8"})
    remove_tracks_for_nets(
        board,
        {
            "Net-(J6-Pin_3)",
            "Net-(J6-Pin_4)",
            "Net-(J6-Pin_5)",
            "Net-(J6-Pin_6)",
            "Net-(J6-Pin_7)",
            "Net-(J6-Pin_8)",
            "Net-(J6-Pin_9)",
        },
    )
    remove_stale_cost_down_stubs(board)
    rename_net_items(board, "Net-(J4-Pin_1)", "/RESERVOIR_SW")
    rename_net_items(board, "Net-(J5-Pin_3)", "/FLOW_PULSE")
    set_pad_net(board, "U1", "8", "/ERROR_LED")
    set_pad_net(board, "U1", "9", "/STATUS_LED")
    for pad_no, net_name in {
        "12": "Net-(U1-IO8)",
        "17": "Net-(U1-IO9)",
        "18": "Net-(U1-IO10)",
        "19": "Net-(U1-IO11)",
        "20": "Net-(U1-IO12)",
    }.items():
        set_pad_net(board, "U1", pad_no, net_name)

    specs = [
        ("R11", "330R error LED", "Resistor_SMD", "R_0805_2012Metric", ids["R11"], 67.0, 99.0, 180, {"1": "/ERROR_LED", "2": "/ERR_LED_A"}),
        ("D2", "ERROR red", "LED_SMD", "LED_0805_2012Metric", ids["D2"], 62.3, 99.0, 0, {"1": "GND", "2": "/ERR_LED_A"}),
        ("R12", "330R status LED", "Resistor_SMD", "R_0805_2012Metric", ids["R12"], 70.5, 102.2, 180, {"1": "/STATUS_LED", "2": "/STAT_LED_A"}),
        ("D3", "STATUS green", "LED_SMD", "LED_0805_2012Metric", ids["D3"], 65.8, 102.2, 0, {"1": "GND", "2": "/STAT_LED_A"}),
        ("TP6", "RESERVOIR_SW optional pad", "TestPoint", "TestPoint_Pad_D1.5mm", ids["TP6"], 98.58, 109.63, 0, {"1": "/RESERVOIR_SW"}),
        ("TP7", "OPT_GND pad", "TestPoint", "TestPoint_Pad_D1.5mm", ids["TP7"], 101.08, 109.63, 0, {"1": "GND"}),
        ("TP8", "OPT_3V3 pad", "TestPoint", "TestPoint_Pad_D1.5mm", ids["TP8"], 86.63, 109.63, 0, {"1": "+3V3"}),
        ("TP9", "OPT_GND pad", "TestPoint", "TestPoint_Pad_D1.5mm", ids["TP9"], 89.13, 109.63, 0, {"1": "GND"}),
        ("TP10", "FLOW_PULSE optional pad", "TestPoint", "TestPoint_Pad_D1.5mm", ids["TP10"], 91.63, 109.63, 0, {"1": "/FLOW_PULSE"}),
    ]
    for ref, value, lib, name, path_uuid, x, y, rot, pad_nets in specs:
        fp = load_fp(lib, name)
        configure_fp(fp, ref, value, lib, name, path_uuid, x, y, rot)
        for pad in fp.Pads():
            net_name = pad_nets.get(pad.GetNumber())
            if net_name:
                pad.SetNet(get_net(board, net_name))
        board.Add(fp)

    add_track(board, "/ERROR_LED", 71.98, 88.11, 68.7, 88.11, 0.18)
    add_track(board, "/ERROR_LED", 68.7, 88.11, 68.7, 99.0, 0.18)
    add_track(board, "/ERROR_LED", 68.7, 99.0, 67.9125, 99.0, 0.18)
    add_track(board, "/ERR_LED_A", 66.0875, 99.0, 63.2375, 99.0, 0.18)
    add_track(board, "/STATUS_LED", 71.98, 89.38, 70.6, 89.38, 0.18)
    add_track(board, "/STATUS_LED", 70.6, 89.38, 70.6, 102.2, 0.18)
    add_track(board, "/STATUS_LED", 70.6, 102.2, 71.4125, 102.2, 0.18)
    add_track(board, "/STAT_LED_A", 69.5875, 102.2, 66.7375, 102.2, 0.18)
    add_text(board, "ERR", 58.8, 97.0, 0.8)
    add_text(board, "STAT", 72.6, 104.8, 0.8)
    add_text(board, "RES OPT", 96.2, 112.6, 0.8)
    add_text(board, "SW GND", 98.0, 117.8, 0.8)
    add_text(board, "FLOW OPT", 84.0, 112.6, 0.8)
    add_text(board, "3V3 GND SIG", 88.0, 117.8, 0.8)

    # Refill all copper zones after the topology change.
    filler = pcbnew.ZONE_FILLER(board)
    filler.Fill(board.Zones())
    pcbnew.SaveBoard(str(PCB), board)


def main() -> None:
    ids = update_schematic()
    update_pcb(ids)


if __name__ == "__main__":
    main()
