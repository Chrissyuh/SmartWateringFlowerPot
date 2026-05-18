from __future__ import annotations

import json
import subprocess
import sys
import uuid
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BOARD_B = ROOT / "board_b_ui"
KICAD = Path(r"C:\Users\chesk\AppData\Local\Programs\KiCad\9.0")
KICAD_BIN = KICAD / "bin" / "kicad-cli.exe"
KICAD_SYMBOLS = KICAD / "share" / "kicad" / "symbols"
KICAD_FOOTPRINTS = KICAD / "share" / "kicad" / "footprints"


PROJECT_NAME = "BoardB_UI"
SHEET_UUID = "a7f04ab3-80e4-4e85-a889-5cd13073f8e9"


SIGNALS = {
    1: "+3V3",
    2: "GND",
    3: "SDA",
    4: "SCL",
    5: "ENC_A",
    6: "ENC_B",
    7: "ENC_SW",
    8: "ERROR_LED",
    9: "STATUS_LED",
}


def new_uuid() -> str:
    return str(uuid.uuid4())


def extract_symbol(library_file: Path, symbol: str, lib_id: str) -> str:
    text = library_file.read_text(encoding="utf-8")
    needle = f'(symbol "{symbol}"'
    start = text.index(needle)
    depth = 0
    in_string = False
    escaped = False
    for index, char in enumerate(text[start:], start):
        if in_string:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
        else:
            if char == '"':
                in_string = True
            elif char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
                if depth == 0:
                    block = text[start : index + 1]
                    return block.replace(f'(symbol "{symbol}"', f'(symbol "{lib_id}"', 1)
    raise RuntimeError(f"Could not extract symbol {symbol} from {library_file}")


def symbol_libs() -> str:
    parts = [
        extract_symbol(KICAD_SYMBOLS / "Connector_Generic.kicad_sym", "Conn_01x09", "Connector_Generic:Conn_01x09"),
        extract_symbol(KICAD_SYMBOLS / "Connector_Generic.kicad_sym", "Conn_01x04", "Connector_Generic:Conn_01x04"),
        extract_symbol(KICAD_SYMBOLS / "Device.kicad_sym", "RotaryEncoder_Switch_MP", "Device:RotaryEncoder_Switch_MP"),
        extract_symbol(KICAD_SYMBOLS / "Device.kicad_sym", "R", "Device:R"),
        extract_symbol(KICAD_SYMBOLS / "Device.kicad_sym", "C", "Device:C"),
        extract_symbol(KICAD_SYMBOLS / "Device.kicad_sym", "LED", "Device:LED"),
        extract_symbol(KICAD_SYMBOLS / "Mechanical.kicad_sym", "MountingHole", "Mechanical:MountingHole"),
    ]
    return "\n".join(parts)


def wire(x1: float, y1: float, x2: float, y2: float) -> str:
    return f'''\t(wire
\t\t(pts
\t\t\t(xy {x1:.2f} {y1:.2f}) (xy {x2:.2f} {y2:.2f})
\t\t)
\t\t(stroke
\t\t\t(width 0)
\t\t\t(type default)
\t\t)
\t\t(uuid "{new_uuid()}")
\t)'''


def label(name: str, x: float, y: float, angle: int = 0, justify: str = "left bottom") -> str:
    return f'''\t(label "{name}"
\t\t(at {x:.2f} {y:.2f} {angle})
\t\t(effects
\t\t\t(font
\t\t\t\t(size 1.27 1.27)
\t\t\t)
\t\t\t(justify {justify})
\t\t)
\t\t(uuid "{new_uuid()}")
\t)'''


def text_note(text: str, x: float, y: float, size: float = 1.27, bold: bool = False) -> str:
    bold_line = "\n\t\t\t\t\t(bold yes)" if bold else ""
    return f'''\t(text "{text}"
\t\t(exclude_from_sim no)
\t\t(at {x:.2f} {y:.2f} 0)
\t\t(effects
\t\t\t(font
\t\t\t\t(size {size:.2f} {size:.2f}){bold_line}
\t\t\t)
\t\t)
\t\t(uuid "{new_uuid()}")
\t)'''


def prop(name: str, value: str, x: float, y: float, angle: int = 0, hide: bool = False) -> str:
    hide_text = " (hide yes)" if hide else ""
    return f'''\t\t(property "{name}" "{value}"
\t\t\t(at {x:.2f} {y:.2f} {angle})
\t\t\t(effects (font (size 1.27 1.27)){hide_text})
\t\t)'''


def instance(ref: str) -> str:
    return f'''\t\t(instances
\t\t\t(project ""
\t\t\t\t(path "/{SHEET_UUID}"
\t\t\t\t\t(reference "{ref}")
\t\t\t\t\t(unit 1)
\t\t\t\t)
\t\t\t)
\t\t)'''


def placed_symbol(
    lib_id: str,
    ref: str,
    value: str,
    footprint: str,
    x: float,
    y: float,
    angle: int = 0,
    pins: list[str] | None = None,
    dnp: bool = False,
    in_bom: bool = True,
) -> str:
    pins = pins or []
    pin_text = "\n".join(f'\t\t(pin "{pin}"\n\t\t\t(uuid "{new_uuid()}")\n\t\t)' for pin in pins)
    return f'''\t(symbol
\t\t(lib_id "{lib_id}")
\t\t(at {x:.2f} {y:.2f} {angle})
\t\t(unit 1)
\t\t(exclude_from_sim no)
\t\t(in_bom {"yes" if in_bom else "no"})
\t\t(on_board yes)
\t\t(dnp {"yes" if dnp else "no"})
\t\t(uuid "{new_uuid()}")
{prop("Reference", ref, x, y - 7.62, angle)}
{prop("Value", value, x, y - 5.08, angle)}
{prop("Footprint", footprint, x, y, angle, hide=True)}
{prop("Datasheet", "~", x, y, angle, hide=True)}
{pin_text}
{instance(ref)}
\t)'''


def left_stub(net: str, x: float, y: float, length: float = 5.08) -> list[str]:
    label_x = x - length
    return [wire(x, y, label_x, y), label(net, label_x, y, 180, "right bottom")]


def right_stub(net: str, x: float, y: float, length: float = 5.08) -> list[str]:
    label_x = x + length
    return [wire(x, y, label_x, y), label(net, label_x, y, 0, "left bottom")]


def make_schematic() -> None:
    items: list[str] = []
    items.append(text_note("Board B UI daughterboard - Rev A", 20.32, 15.24, 1.50, True))
    items.append(text_note("J1 matches Board A J6 exactly. OLED module pin order is GND, VCC, SCL, SDA.", 20.32, 20.32))
    items.append(text_note("Verify actual OLED module pin order and encoder footprint before fabrication.", 20.32, 25.40))

    # J1 UI cable connector.
    j1x, j1y = 50.80, 58.42
    items.append(
        placed_symbol(
            "Connector_Generic:Conn_01x09",
            "J1",
            "UI cable to Board A J6",
            "Connector_JST:JST_XH_S9B-XH-A_1x09_P2.50mm_Horizontal",
            j1x,
            j1y,
            pins=[str(i) for i in range(1, 10)],
        )
    )
    for pin, net in SIGNALS.items():
        y = j1y - 12.70 + (2.54 * pin)
        items.extend(left_stub(net, j1x - 5.08, y, 7.62))
        items.append(text_note(f"J1-{pin}: {net}", 20.32, y - 0.76, 1.00))

    # OLED header.
    j2x, j2y = 104.14, 35.56
    oled_pins = {1: "GND", 2: "+3V3", 3: "SCL", 4: "SDA"}
    items.append(
        placed_symbol(
            "Connector_Generic:Conn_01x04",
            "J2",
            "OLED SSD1306 I2C",
            "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical",
            j2x,
            j2y,
            pins=["1", "2", "3", "4"],
        )
    )
    for pin, net in oled_pins.items():
        y = j2y - 5.08 + (2.54 * pin)
        items.extend(left_stub(net, j2x - 5.08, y))
    items.append(text_note("J2 OLED: 1=GND, 2=VCC/3V3, 3=SCL, 4=SDA", 91.44, 48.26, 1.00))

    # Encoder with switch and mounting pins.
    swx, swy = 104.14, 72.39
    items.append(
        placed_symbol(
            "Device:RotaryEncoder_Switch_MP",
            "SW1",
            "EC11 encoder + push",
            "Rotary_Encoder:RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm",
            swx,
            swy,
            pins=["A", "C", "B", "MP", "S1", "S2"],
        )
    )
    items.extend(left_stub("ENC_A", swx - 7.62, swy - 2.54))
    items.extend(left_stub("GND", swx - 7.62, swy))
    items.extend(left_stub("ENC_B", swx - 7.62, swy + 2.54))
    items.extend(right_stub("ENC_SW", swx + 7.62, swy - 2.54))
    items.extend(right_stub("GND", swx + 7.62, swy + 2.54))
    items.extend(right_stub("GND", swx, swy + 7.62))

    # Pullups and optional debounce capacitors.
    pullups = [
        ("R1", "10k ENC_A pullup", "ENC_A", 137.16, 45.72),
        ("R2", "10k ENC_B pullup", "ENC_B", 137.16, 58.42),
        ("R3", "10k ENC_SW pullup", "ENC_SW", 137.16, 71.12),
    ]
    caps = [
        ("C1", "100nF ENC_A debounce DNP", "ENC_A", 157.48, 45.72),
        ("C2", "100nF ENC_B debounce DNP", "ENC_B", 157.48, 58.42),
        ("C3", "100nF ENC_SW debounce DNP", "ENC_SW", 157.48, 71.12),
    ]
    for ref, value, net, x, y in pullups:
        items.append(placed_symbol("Device:R", ref, value, "Resistor_SMD:R_0805_2012Metric", x, y, pins=["1", "2"]))
        items.extend(left_stub("+3V3", x, y - 3.81))
        items.extend(left_stub(net, x, y + 3.81))
    for ref, value, net, x, y in caps:
        items.append(placed_symbol("Device:C", ref, value, "Capacitor_SMD:C_0805_2012Metric", x, y, pins=["1", "2"], dnp=True))
        items.extend(right_stub(net, x, y - 3.81))
        items.extend(right_stub("GND", x, y + 3.81))

    # LED circuits. Active-high: GPIO signal -> resistor -> LED anode -> LED cathode -> GND.
    led_rows = [
        ("R4", "330R error LED", "ERROR_LED", "D1", "ERROR red", "ERR_LED_A", 137.16, 88.90),
        ("R5", "330R status LED", "STATUS_LED", "D2", "STATUS green/blue", "STAT_LED_A", 137.16, 101.60),
    ]
    for rref, rval, sig, dref, dval, anode_net, x, y in led_rows:
        items.append(placed_symbol("Device:R", rref, rval, "Resistor_SMD:R_0805_2012Metric", x, y, pins=["1", "2"]))
        items.extend(left_stub(sig, x, y - 3.81))
        items.extend(left_stub(anode_net, x, y + 3.81))
        items.append(placed_symbol("Device:LED", dref, dval, "LED_SMD:LED_0805_2012Metric", x + 20.32, y + 3.81, pins=["1", "2"]))
        items.extend(left_stub("GND", x + 20.32 - 3.81, y + 3.81))
        items.extend(right_stub(anode_net, x + 20.32 + 3.81, y + 3.81))

    for ref, x, y in [
        ("H1", 137.16, 119.38),
        ("H2", 149.86, 119.38),
        ("H3", 162.56, 119.38),
        ("H4", 175.26, 119.38),
    ]:
        items.append(
            placed_symbol(
                "Mechanical:MountingHole",
                ref,
                "M2.5 mounting hole",
                "MountingHole:MountingHole_2.7mm_M2.5",
                x,
                y,
                in_bom=False,
            )
        )

    body = "\n".join(items)
    sch = f'''(kicad_sch
\t(version 20250114)
\t(generator "eeschema")
\t(generator_version "9.0")
\t(uuid "{SHEET_UUID}")
\t(paper "A4")
\t(lib_symbols
{symbol_libs()}
\t)
{body}
\t(sheet_instances
\t\t(path "/"
\t\t\t(page "1")
\t\t)
\t)
\t(embedded_fonts no)
)
'''
    (BOARD_B / f"{PROJECT_NAME}.kicad_sch").write_text(sch, encoding="utf-8")


def mm(value: float):
    import pcbnew

    return pcbnew.FromMM(value)


def vec(x: float, y: float):
    import pcbnew

    return pcbnew.VECTOR2I(mm(x), mm(y))


def make_project_file() -> None:
    source = ROOT / "SmartWateringFlowerPot.kicad_pro"
    data = json.loads(source.read_text(encoding="utf-8"))
    data["meta"]["filename"] = f"{PROJECT_NAME}.kicad_pro"
    data["sheets"] = [[SHEET_UUID, "Root"]]
    (BOARD_B / f"{PROJECT_NAME}.kicad_pro").write_text(json.dumps(data, indent=2), encoding="utf-8")


def make_readme() -> None:
    readme = """# Board B UI Daughterboard

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

The first PCB outline is 64 mm x 38 mm with M2.5 non-plated mounting holes, a right-angle JST-XH cable connector, EC11 encoder footprint, optional DNP debounce capacitors, and active-high front-panel LEDs.

Board B has explicit routed copper for every net plus front/back `GND` zones. Refill zones in KiCad with `B` after manual edits and rerun DRC before fabrication.

The KiCad footprint references an EC11 3D model under `Rotary_Encoder.3dshapes`. This KiCad install does not include that model, so the STEP export may omit the encoder body even though the footprint, holes, and courtyard are present.
"""
    (BOARD_B / "README.md").write_text(readme, encoding="utf-8")


def make_board() -> None:
    import pcbnew

    board = pcbnew.BOARD()

    nets = {}

    def net(name: str):
        board_name = name if name.startswith("/") else f"/{name}"
        if board_name not in nets:
            item = pcbnew.NETINFO_ITEM(board, board_name)
            board.Add(item)
            nets[board_name] = item
        return nets[board_name]

    net_names = ["+3V3", "GND", "SDA", "SCL", "ENC_A", "ENC_B", "ENC_SW", "ERROR_LED", "STATUS_LED", "ERR_LED_A", "STAT_LED_A"]
    for name in net_names:
        net(name)

    def load_fp(lib: str, name: str, ref: str, value: str, x: float, y: float, rot: float = 0.0):
        fp = pcbnew.FootprintLoad(str(KICAD_FOOTPRINTS / f"{lib}.pretty"), name)
        if fp is None:
            raise RuntimeError(f"Could not load footprint {lib}:{name}")
        fp.SetFPIDAsString(f"{lib}:{name}")
        fp.SetReference(ref)
        fp.SetValue(value)
        fp.SetPosition(vec(x, y))
        fp.SetOrientationDegrees(rot)
        board.Add(fp)
        return fp

    footprints = {}
    footprints["J1"] = load_fp("Connector_JST", "JST_XH_S9B-XH-A_1x09_P2.50mm_Horizontal", "J1", "UI cable to Board A J6", 21.0, 28.0)
    footprints["J2"] = load_fp("Connector_PinHeader_2.54mm", "PinHeader_1x04_P2.54mm_Vertical", "J2", "OLED SSD1306 I2C", 10.0, 8.0)
    footprints["SW1"] = load_fp("Rotary_Encoder", "RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm", "SW1", "EC11 encoder + push", 40.0, 15.0)

    for ref, x, y, rot in [
        ("R1", 23.5, 7.6, 0),
        ("R2", 23.5, 11.8, 0),
        ("R3", 23.5, 16.0, 0),
        ("R4", 45.0, 7.4, 90),
        ("R5", 52.0, 7.4, 90),
    ]:
        footprints[ref] = load_fp("Resistor_SMD", "R_0805_2012Metric", ref, {
            "R1": "10k ENC_A pullup",
            "R2": "10k ENC_B pullup",
            "R3": "10k ENC_SW pullup",
            "R4": "330R error LED",
            "R5": "330R status LED",
        }[ref], x, y, rot)

    for ref, x, y, rot in [
        ("C1", 27.5, 6.65, 90),
        ("C2", 27.5, 10.85, 90),
        ("C3", 27.5, 15.05, 90),
    ]:
        fp = load_fp("Capacitor_SMD", "C_0805_2012Metric", ref, {
            "C1": "100nF ENC_A debounce DNP",
            "C2": "100nF ENC_B debounce DNP",
            "C3": "100nF ENC_SW debounce DNP",
        }[ref], x, y, rot)
        fp.SetDNP(True)
        footprints[ref] = fp

    footprints["D1"] = load_fp("LED_SMD", "LED_0805_2012Metric", "D1", "ERROR red", 45.0, 4.2)
    footprints["D2"] = load_fp("LED_SMD", "LED_0805_2012Metric", "D2", "STATUS green/blue", 52.0, 4.2)

    for ref, x, y in [("H1", 4, 4), ("H2", 60, 4), ("H3", 4, 34), ("H4", 60, 34)]:
        footprints[ref] = load_fp("MountingHole", "MountingHole_2.7mm_M2.5", ref, "M2.5 mounting hole", x, y)

    pad_nets = {
        "J1": {
            "1": "+3V3",
            "2": "GND",
            "3": "SDA",
            "4": "SCL",
            "5": "ENC_A",
            "6": "ENC_B",
            "7": "ENC_SW",
            "8": "ERROR_LED",
            "9": "STATUS_LED",
        },
        "J2": {"1": "GND", "2": "+3V3", "3": "SCL", "4": "SDA"},
        "SW1": {"A": "ENC_A", "B": "ENC_B", "C": "GND", "S1": "ENC_SW", "S2": "GND", "MP": "GND"},
        "R1": {"1": "+3V3", "2": "ENC_A"},
        "R2": {"1": "+3V3", "2": "ENC_B"},
        "R3": {"1": "+3V3", "2": "ENC_SW"},
        "C1": {"1": "ENC_A", "2": "GND"},
        "C2": {"1": "ENC_B", "2": "GND"},
        "C3": {"1": "ENC_SW", "2": "GND"},
        "R4": {"1": "ERROR_LED", "2": "ERR_LED_A"},
        "D1": {"1": "GND", "2": "ERR_LED_A"},
        "R5": {"1": "STATUS_LED", "2": "STAT_LED_A"},
        "D2": {"1": "GND", "2": "STAT_LED_A"},
    }

    def pads(fp):
        by_number = {}
        for pad in fp.Pads():
            by_number.setdefault(str(pad.GetNumber()), []).append(pad)
        return by_number

    fp_pads = {ref: pads(fp) for ref, fp in footprints.items()}
    for ref, mapping in pad_nets.items():
        for pad_name, net_name in mapping.items():
            for pad in fp_pads[ref][pad_name]:
                pad.SetNet(net(net_name))

    for ref in ["R1", "R2", "R3", "R4", "R5", "C1", "C2", "C3", "D1", "D2", "H1", "H2", "H3", "H4"]:
        footprints[ref].Reference().SetVisible(False)
        footprints[ref].Value().SetVisible(False)

    def add_track(points, net_name: str, layer=None, width: float = 0.25):
        if layer is None:
            layer = pcbnew.F_Cu
        for a, b in zip(points, points[1:]):
            segment = pcbnew.PCB_TRACK(board)
            segment.SetStart(vec(*a))
            segment.SetEnd(vec(*b))
            segment.SetWidth(mm(width))
            segment.SetLayer(layer)
            segment.SetNet(net(net_name))
            board.Add(segment)

    def add_via(x: float, y: float, net_name: str):
        via = pcbnew.PCB_VIA(board)
        via.SetPosition(vec(x, y))
        via.SetWidth(mm(0.8))
        via.SetDrill(mm(0.4))
        via.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
        via.SetNet(net(net_name))
        board.Add(via)

    def pad_xy(ref: str, pad_name: str) -> tuple[float, float]:
        pos = fp_pads[ref][pad_name][0].GetPosition()
        return pcbnew.ToMM(pos.x), pcbnew.ToMM(pos.y)

    def pad_xy_n(ref: str, pad_name: str, index: int) -> tuple[float, float]:
        pos = fp_pads[ref][pad_name][index].GetPosition()
        return pcbnew.ToMM(pos.x), pcbnew.ToMM(pos.y)

    def route_pad_to(point: tuple[float, float], ref: str, pad_name: str, net_name: str, layer=None):
        p = pad_xy(ref, pad_name)
        add_track([p, point], net_name, layer=layer or pcbnew.F_Cu)

    def route(points, net_name: str, layer=None, width: float = 0.25):
        add_track(points, net_name, layer=layer or pcbnew.F_Cu, width=width)

    def route_pad(ref: str, pad_name: str, points: list[tuple[float, float]], net_name: str, layer=None, width: float = 0.25):
        route([pad_xy(ref, pad_name), *points], net_name, layer=layer, width=width)

    def route_between(start_ref: str, start_pad: str, points: list[tuple[float, float]], end_ref: str, end_pad: str, net_name: str, layer=None, width: float = 0.25):
        route([pad_xy(start_ref, start_pad), *points, pad_xy(end_ref, end_pad)], net_name, layer=layer, width=width)

    # Local pullup/debounce routing.
    for rref, cref, net_name, y in [
        ("R1", "C1", "ENC_A", 7.6),
        ("R2", "C2", "ENC_B", 11.8),
        ("R3", "C3", "ENC_SW", 16.0),
    ]:
        route_between(rref, "2", [(25.5, y), (26.4, y)], cref, "1", net_name)
        route_pad(rref, "1", [(21.8, y)], "+3V3", width=0.3)
        route_pad(cref, "2", [(30.5, y - 1.9)], "GND", width=0.3)

    route([(21.8, 7.6), (21.8, 11.8), (21.8, 16.0)], "+3V3", width=0.3)
    route([(30.5, 5.7), (30.5, 9.9), (30.5, 14.1), (30.5, 17.5), (40.0, 17.5)], "GND", width=0.35)

    # Power and OLED header.
    route_between("J2", "2", [(14.0, 10.54), (17.0, 7.6), (21.8, 7.6)], "R1", "1", "+3V3", width=0.3)
    route_pad("J1", "1", [(18.0, 25.0), (17.0, 17.0), (21.8, 16.0)], "+3V3", width=0.3)
    route_between("J2", "1", [(6.0, 8.0), (6.0, 31.5), (23.5, 31.5)], "J1", "2", "GND", layer=pcbnew.B_Cu, width=0.35)
    route_pad("J1", "2", [(26.0, 25.0), (30.5, 20.5), (30.5, 17.5)], "GND", width=0.35)

    # I2C from Board A cable to OLED header.  Long runs stay on B.Cu.
    route_between("J1", "3", [(21.0, 23.0), (12.0, 16.7)], "J2", "4", "SDA", layer=pcbnew.B_Cu)
    route_between("J1", "4", [(23.0, 23.0), (12.0, 14.1)], "J2", "3", "SCL", layer=pcbnew.B_Cu)

    # Encoder nets.  The route uses B.Cu for crossings, with short local F.Cu
    # stubs at SMD debounce footprints.
    add_via(29.4, 7.6, "ENC_A")
    route_pad("C1", "1", [(29.4, 7.6)], "ENC_A")
    route([(29.4, 7.6), (33.5, 8.8), (38.0, 13.0), (40.0, 15.0)], "ENC_A", layer=pcbnew.B_Cu)
    route([(40.0, 15.0), (37.0, 18.0), (32.2, 25.8), (31.0, 28.0)], "ENC_A", layer=pcbnew.B_Cu)

    add_via(29.4, 11.8, "ENC_B")
    route_pad("C2", "1", [(29.4, 11.8)], "ENC_B")
    route([(29.4, 11.8), (32.0, 12.0)], "ENC_B", layer=pcbnew.B_Cu)
    add_via(32.0, 12.0, "ENC_B")
    route([(32.0, 12.0), (42.0, 12.0)], "ENC_B")
    add_via(42.0, 12.0, "ENC_B")
    route([(42.0, 12.0), (42.0, 20.0), (40.0, 20.0)], "ENC_B", layer=pcbnew.B_Cu)
    route([(40.0, 20.0), (37.0, 23.0), (34.7, 26.8), (33.5, 28.0)], "ENC_B", layer=pcbnew.B_Cu)

    add_via(29.4, 16.0, "ENC_SW")
    route_pad("C3", "1", [(29.4, 16.0)], "ENC_SW")
    route([(29.4, 16.0), (32.5, 19.0)], "ENC_SW", layer=pcbnew.B_Cu)
    add_via(32.5, 19.0, "ENC_SW")
    route([(32.5, 19.0), (34.0, 23.0), (36.0, 28.0)], "ENC_SW")
    route([(36.0, 28.0), (38.0, 26.5), (57.5, 26.5), (57.5, 23.0), (54.5, 20.0)], "ENC_SW", layer=pcbnew.B_Cu)

    # Encoder grounded common, switch return, and mounting lugs.
    route_between("SW1", "C", [(44.0, 17.5), (49.0, 15.0)], "SW1", "S2", "GND", width=0.35)
    route_between("SW1", "S2", [(51.0, 13.0)], "SW1", "MP", "GND", width=0.35)
    route([pad_xy("SW1", "C"), (43.0, 21.5), pad_xy_n("SW1", "MP", 1)], "GND", width=0.35)

    # Active-high LEDs.
    route_between("R4", "2", [(45.9, 6.1)], "D1", "2", "ERR_LED_A")
    route_between("R5", "2", [(52.9, 6.1)], "D2", "2", "STAT_LED_A")
    route_between("D1", "1", [(41.5, 4.2), (36.0, 5.7), (30.5, 5.7)], "C1", "2", "GND", width=0.3)
    route([pad_xy("D2", "1"), (51.0, 2.8), (44.1, 2.8), pad_xy("D1", "1")], "GND", width=0.3)

    add_via(45.0, 9.7, "ERROR_LED")
    route_pad("R4", "1", [(45.0, 9.7)], "ERROR_LED")
    route([(38.5, 28.0), (38.5, 31.0), (42.0, 32.0), (62.0, 32.0), (62.0, 9.7), (45.0, 9.7)], "ERROR_LED", layer=pcbnew.B_Cu)

    add_via(52.0, 10.7, "STATUS_LED")
    route_pad("R5", "1", [(52.0, 10.7)], "STATUS_LED")
    route([(41.0, 28.0), (41.0, 30.0), (60.0, 30.0), (60.0, 10.7), (52.0, 10.7)], "STATUS_LED", layer=pcbnew.B_Cu)

    # Board outline.
    for a, b in [((0, 0), (64, 0)), ((64, 0), (64, 38)), ((64, 38), (0, 38)), ((0, 38), (0, 0))]:
        shape = pcbnew.PCB_SHAPE(board)
        shape.SetShape(pcbnew.SHAPE_T_SEGMENT)
        shape.SetStart(vec(*a))
        shape.SetEnd(vec(*b))
        shape.SetWidth(mm(0.05))
        shape.SetLayer(pcbnew.Edge_Cuts)
        board.Add(shape)

    # Front/back GND pours inside the outline.
    for layer in [pcbnew.F_Cu, pcbnew.B_Cu]:
        zone = pcbnew.ZONE(board)
        zone.SetLayer(layer)
        zone.SetNet(net("GND"))
        zone.SetLocalClearance(mm(0.25))
        zone.SetFillFlag(layer, True)
        zone.SetIsFilled(False)
        outline = zone.Outline()
        outline.NewOutline()
        for x, y in [(0.6, 0.6), (63.4, 0.6), (63.4, 37.4), (0.6, 37.4)]:
            outline.Append(vec(x, y))
        board.Add(zone)

    # Silkscreen labels.
    def add_text(txt: str, x: float, y: float, size: float = 1.0, layer=None):
        layer = layer or pcbnew.F_SilkS
        item = pcbnew.PCB_TEXT(board)
        item.SetText(txt)
        item.SetPosition(vec(x, y))
        item.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size)))
        item.SetTextThickness(mm(0.12))
        item.SetLayer(layer)
        board.Add(item)

    add_text("BOARD B UI", 10.0, 2.2, 1.2)
    add_text("J1 TO BOARD A", 14.0, 24.0, 0.8)
    add_text("OLED GND VCC SCL SDA", 14.0, 18.5, 0.8)
    add_text("ERR", 43.0, 2.2, 0.8)
    add_text("STAT", 49.8, 2.2, 0.8)
    add_text("EC11", 58.0, 25.4, 1.0)
    add_text("J1 1=3V3 2=GND 3=SDA 4=SCL 5=A 6=B 7=SW 8=ERR 9=STAT", 4.0, 35.0, 0.8, pcbnew.Cmts_User)

    pcb_path = BOARD_B / f"{PROJECT_NAME}.kicad_pcb"
    pcbnew.SaveBoard(str(pcb_path), board)

    # KiCad 9's in-process zone filler can crash when called before the newly
    # generated board is reloaded.  Fill in a fresh KiCad Python process so the
    # committed board has real GND pours instead of only zone outlines.
    fill_script = (
        "import pcbnew; "
        f"path={str(pcb_path)!r}; "
        "board=pcbnew.LoadBoard(path); "
        "pcbnew.ZONE_FILLER(board).Fill(board.Zones()); "
        "pcbnew.SaveBoard(path, board)"
    )
    subprocess.run([sys.executable, "-c", fill_script], check=False)


def run_checks() -> None:
    reports = BOARD_B / "reports"
    outputs = BOARD_B / "outputs"
    mechanical = BOARD_B / "mechanical"
    reports.mkdir(exist_ok=True)
    outputs.mkdir(exist_ok=True)
    mechanical.mkdir(exist_ok=True)

    sch = BOARD_B / f"{PROJECT_NAME}.kicad_sch"
    pcb = BOARD_B / f"{PROJECT_NAME}.kicad_pcb"
    subprocess.run([str(KICAD_BIN), "sch", "erc", "--format", "report", "--output", str(reports / "erc-board-b.txt"), str(sch)], check=False)
    subprocess.run([str(KICAD_BIN), "sch", "export", "pdf", "--output", str(outputs / "BoardB_UI-schematic.pdf"), str(sch)], check=False)
    subprocess.run([str(KICAD_BIN), "pcb", "drc", "--format", "report", "--output", str(reports / "drc-board-b.txt"), str(pcb)], check=False)
    subprocess.run([str(KICAD_BIN), "pcb", "drc", "--schematic-parity", "--format", "report", "--output", str(reports / "drc-board-b-parity.txt"), str(pcb)], check=False)
    subprocess.run([str(KICAD_BIN), "pcb", "export", "step", "--output", str(mechanical / "BoardB_UI.step"), str(pcb)], check=False)


def main() -> int:
    BOARD_B.mkdir(exist_ok=True)
    (BOARD_B / "reports").mkdir(exist_ok=True)
    (BOARD_B / "outputs").mkdir(exist_ok=True)
    (BOARD_B / "mechanical").mkdir(exist_ok=True)
    make_project_file()
    make_readme()
    make_schematic()
    make_board()
    run_checks()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
