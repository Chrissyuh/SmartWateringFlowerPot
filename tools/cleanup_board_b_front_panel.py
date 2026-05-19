from __future__ import annotations

from pathlib import Path
import subprocess
import sys

import pcbnew


ROOT = Path(__file__).resolve().parents[1]
BOARD_PATH = ROOT / "board_b_ui" / "BoardB_UI.kicad_pcb"
KICAD_FOOTPRINTS = Path(r"C:\Users\chesk\AppData\Local\Programs\KiCad\9.0\share\kicad\footprints")


def mm(value: float) -> int:
    return pcbnew.FromMM(value)


def to_mm(value: int) -> float:
    return pcbnew.ToMM(value)


def vec(x: float, y: float) -> pcbnew.VECTOR2I:
    return pcbnew.VECTOR2I(mm(x), mm(y))


def set_footprint(fp: pcbnew.FOOTPRINT, x: float, y: float, rotation: float = 0.0, back: bool = False) -> None:
    target_layer = pcbnew.B_Cu if back else pcbnew.F_Cu
    if fp.IsFlipped() != back:
        fp.SetLayerAndFlip(target_layer)
    fp.SetPosition(vec(x, y))
    fp.SetOrientationDegrees(rotation)
    fp.SetPosition(vec(x, y))


def pad_xy(footprints: dict[str, pcbnew.FOOTPRINT], ref: str, pad_name: str, index: int = 0) -> tuple[float, float]:
    pads = [pad for pad in footprints[ref].Pads() if str(pad.GetNumber()) == pad_name]
    if not pads:
        raise RuntimeError(f"{ref} pad {pad_name} was not found")
    if index >= len(pads):
        raise RuntimeError(f"{ref} pad {pad_name} index {index} was not found")
    pos = pads[index].GetPosition()
    return to_mm(pos.x), to_mm(pos.y)


def add_track(
    board: pcbnew.BOARD,
    net: pcbnew.NETINFO_ITEM,
    points: list[tuple[float, float]],
    layer: int,
    width: float = 0.25,
) -> None:
    for start, end in zip(points, points[1:]):
        segment = pcbnew.PCB_TRACK(board)
        segment.SetStart(vec(*start))
        segment.SetEnd(vec(*end))
        segment.SetLayer(layer)
        segment.SetWidth(mm(width))
        segment.SetNet(net)
        board.Add(segment)


def add_via(
    board: pcbnew.BOARD,
    net: pcbnew.NETINFO_ITEM,
    x: float,
    y: float,
    drill: float = 0.4,
    diameter: float = 0.8,
) -> None:
    via = pcbnew.PCB_VIA(board)
    via.SetPosition(vec(x, y))
    via.SetDrill(mm(drill))
    via.SetWidth(mm(diameter))
    via.SetLayerPair(pcbnew.F_Cu, pcbnew.B_Cu)
    via.SetNet(net)
    board.Add(via)


def add_rect(board: pcbnew.BOARD, x1: float, y1: float, x2: float, y2: float, layer: int, width: float = 0.12) -> None:
    for start, end in [((x1, y1), (x2, y1)), ((x2, y1), (x2, y2)), ((x2, y2), (x1, y2)), ((x1, y2), (x1, y1))]:
        shape = pcbnew.PCB_SHAPE(board)
        shape.SetShape(pcbnew.SHAPE_T_SEGMENT)
        shape.SetStart(vec(*start))
        shape.SetEnd(vec(*end))
        shape.SetLayer(layer)
        shape.SetWidth(mm(width))
        board.Add(shape)


def add_text(
    board: pcbnew.BOARD,
    text: str,
    x: float,
    y: float,
    size: float = 1.0,
    layer: int = pcbnew.F_SilkS,
    angle: float = 0.0,
) -> None:
    item = pcbnew.PCB_TEXT(board)
    item.SetText(text)
    item.SetPosition(vec(x, y))
    item.SetTextSize(pcbnew.VECTOR2I(mm(size), mm(size)))
    item.SetTextThickness(mm(0.12))
    item.SetLayer(layer)
    item.SetTextAngle(pcbnew.EDA_ANGLE(angle, pcbnew.DEGREES_T))
    board.Add(item)


def add_zone(board: pcbnew.BOARD, net: pcbnew.NETINFO_ITEM, layer: int) -> None:
    zone = pcbnew.ZONE(board)
    zone.SetLayer(layer)
    zone.SetNet(net)
    zone.SetLocalClearance(mm(0.25))
    zone.SetFillFlag(layer, True)
    zone.SetIsFilled(False)
    outline = zone.Outline()
    outline.NewOutline()
    for x, y in [(0.6, 0.6), (63.4, 0.6), (63.4, 37.4), (0.6, 37.4)]:
        outline.Append(vec(x, y))
    board.Add(zone)


def main() -> int:
    board = pcbnew.BOARD()

    nets: dict[str, pcbnew.NETINFO_ITEM] = {}

    def add_net(name: str) -> pcbnew.NETINFO_ITEM:
        net_name = name if name.startswith("/") else f"/{name}"
        item = pcbnew.NETINFO_ITEM(board, net_name)
        board.Add(item)
        nets[name] = item
        return item

    for name in ["+3V3", "GND", "SDA", "SCL", "ENC_A", "ENC_B", "ENC_SW", "ERROR_LED", "STATUS_LED", "ERR_LED_A", "STAT_LED_A"]:
        add_net(name)

    def load_fp(lib: str, name: str, ref: str, value: str, x: float, y: float, rotation: float = 0.0, back: bool = False) -> pcbnew.FOOTPRINT:
        footprint = pcbnew.FootprintLoad(str(KICAD_FOOTPRINTS / f"{lib}.pretty"), name)
        if footprint is None:
            raise RuntimeError(f"Could not load {lib}:{name}")
        footprint.SetFPIDAsString(f"{lib}:{name}")
        footprint.SetReference(ref)
        footprint.SetValue(value)
        board.Add(footprint)
        set_footprint(footprint, x, y, rotation, back=back)
        return footprint

    footprints = {
        "J1": load_fp("Connector_JST", "JST_XH_S9B-XH-A_1x09_P2.50mm_Horizontal", "J1", "UI cable to Board A J6", 20.0, 34.0, back=True),
        "J2": load_fp("Connector_PinHeader_2.54mm", "PinHeader_1x04_P2.54mm_Vertical", "J2", "OLED SSD1306 I2C", 12.0, 10.0),
        "SW1": load_fp("Rotary_Encoder", "RotaryEncoder_Alps_EC11E-Switch_Vertical_H20mm", "SW1", "EC11 encoder + push", 44.0, 20.0),
        "D1": load_fp("LED_SMD", "LED_0805_2012Metric", "D1", "ERROR red", 43.0, 7.0),
        "D2": load_fp("LED_SMD", "LED_0805_2012Metric", "D2", "STATUS green/blue", 52.0, 7.0),
        "R1": load_fp("Resistor_SMD", "R_0805_2012Metric", "R1", "10k ENC_A pullup", 30.0, 12.0, 270, back=True),
        "R2": load_fp("Resistor_SMD", "R_0805_2012Metric", "R2", "10k ENC_B pullup", 32.5, 12.0, 270, back=True),
        "R3": load_fp("Resistor_SMD", "R_0805_2012Metric", "R3", "10k ENC_SW pullup", 35.0, 12.0, 270, back=True),
        "C1": load_fp("Capacitor_SMD", "C_0805_2012Metric", "C1", "100nF ENC_A debounce DNP", 30.0, 17.0, 90, back=True),
        "C2": load_fp("Capacitor_SMD", "C_0805_2012Metric", "C2", "100nF ENC_B debounce DNP", 32.5, 17.0, 90, back=True),
        "C3": load_fp("Capacitor_SMD", "C_0805_2012Metric", "C3", "100nF ENC_SW debounce DNP", 35.0, 17.0, 90, back=True),
        "R4": load_fp("Resistor_SMD", "R_0805_2012Metric", "R4", "330R error LED", 43.0, 10.6, 90, back=True),
        "R5": load_fp("Resistor_SMD", "R_0805_2012Metric", "R5", "330R status LED", 52.0, 10.6, 90, back=True),
        "H1": load_fp("MountingHole", "MountingHole_2.7mm_M2.5", "H1", "M2.5 mounting hole", 4.0, 4.0),
        "H2": load_fp("MountingHole", "MountingHole_2.7mm_M2.5", "H2", "M2.5 mounting hole", 60.0, 4.0),
        "H3": load_fp("MountingHole", "MountingHole_2.7mm_M2.5", "H3", "M2.5 mounting hole", 4.0, 34.0),
        "H4": load_fp("MountingHole", "MountingHole_2.7mm_M2.5", "H4", "M2.5 mounting hole", 60.0, 34.0),
    }

    for ref in ["C1", "C2", "C3"]:
        footprints[ref].SetDNP(True)

    pad_nets = {
        "J1": {"1": "+3V3", "2": "GND", "3": "SDA", "4": "SCL", "5": "ENC_A", "6": "ENC_B", "7": "ENC_SW", "8": "ERROR_LED", "9": "STATUS_LED"},
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

    for ref, mapping in pad_nets.items():
        for pad_name, net_name in mapping.items():
            for footprint_pad in footprints[ref].Pads():
                if str(footprint_pad.GetNumber()) == pad_name:
                    footprint_pad.SetNet(nets[net_name])

    for ref, fp in footprints.items():
        fp.Reference().SetVisible(ref in {"J2", "SW1", "D1", "D2"})
        fp.Value().SetVisible(False)

    def p(ref: str, pad: str, index: int = 0) -> tuple[float, float]:
        return pad_xy(footprints, ref, pad, index)

    def route(net_name: str, points: list[tuple[float, float]], layer: int = pcbnew.B_Cu, width: float = 0.25) -> None:
        add_track(board, nets[net_name], points, layer, width)

    def via(net_name: str, x: float, y: float) -> None:
        add_via(board, nets[net_name], x, y)

    # Power stays on the back.  I2C uses the front side so it can pass cleanly
    # under the OLED module without crossing the back-side power rail.
    route("+3V3", [p("J1", "1"), (17.0, 34.0), (17.0, 13.0), p("J2", "2")], width=0.3)
    route("+3V3", [(17.0, 13.0), (26.0, 10.088), p("R1", "1"), p("R2", "1"), p("R3", "1")], width=0.3)
    route("SDA", [p("J1", "3"), (22.0, 30.0), (15.0, 17.62), p("J2", "4")], layer=pcbnew.F_Cu)
    route("SCL", [p("J1", "4"), (27.5, 25.0), (15.0, 15.08), p("J2", "3")], layer=pcbnew.F_Cu)

    # Encoder pullups and optional debounce capacitors.
    route("ENC_A", [p("J1", "5"), p("C1", "1"), (28.6, 17.95), (28.6, 13.912), p("R1", "2")])
    route("ENC_B", [p("J1", "6"), p("C2", "1"), (31.1, 17.95), (31.1, 13.912), p("R2", "2")])
    route("ENC_SW", [p("J1", "7"), p("C3", "1"), (36.4, 17.95), (36.4, 13.912), p("R3", "2")])
    via("GND", *p("C1", "2"))

    # Front-side encoder signal runs stay separated by y-lane.
    route("ENC_A", [p("J1", "5"), (30.0, 20.0), p("SW1", "A")], layer=pcbnew.F_Cu)
    route("ENC_B", [p("J1", "6"), (32.5, 25.0), p("SW1", "B")], layer=pcbnew.F_Cu)
    route("ENC_SW", [p("J1", "7"), (35.0, 31.5), (58.5, 31.5), p("SW1", "S1")], layer=pcbnew.F_Cu)

    # Active-high LED drive: Board A signal -> back-side resistor -> via -> front LED.
    route("ERROR_LED", [p("J1", "8"), (37.5, 32.0), (39.0, 32.0), (39.0, 12.0), p("R4", "1")])
    route("STATUS_LED", [p("J1", "9"), (55.0, 34.0), (55.0, 12.0), p("R5", "1")])
    for net_name, rref, dref, via_xy in [
        ("ERR_LED_A", "R4", "D1", (45.2, 8.5)),
        ("STAT_LED_A", "R5", "D2", (54.2, 8.5)),
    ]:
        route(net_name, [p(rref, "2"), via_xy])
        via(net_name, *via_xy)
        route(net_name, [via_xy, p(dref, "2")], layer=pcbnew.F_Cu)

    # Board outline, front OLED module outline, and readable labels.
    add_rect(board, 0, 0, 64, 38, pcbnew.Edge_Cuts, width=0.05)
    add_rect(board, 6.5, 5.2, 34.8, 33.4, pcbnew.F_Fab, width=0.10)
    add_rect(board, 7.8, 6.5, 33.5, 23.5, pcbnew.F_Fab, width=0.10)
    add_text(board, "OLED 0.96 SSD1306", 8.0, 3.1, 0.8, layer=pcbnew.F_Fab)
    add_text(board, "GND VCC SCL SDA", 7.0, 31.7, 0.8)
    add_text(board, "ERR", 40.7, 3.2, 0.8)
    add_text(board, "STAT", 49.0, 3.2, 0.8)
    add_text(board, "SW1", 53.0, 31.5, 0.9)
    add_text(board, "BOARD B UI", 21.0, 2.0, 1.0)
    add_text(board, "J1 TO BOARD A", 18.5, 36.7, 0.8, layer=pcbnew.Cmts_User)
    add_text(board, "1=3V3 2=GND 3=SDA 4=SCL 5=A 6=B 7=SW 8=ERR 9=STAT", 3.0, 36.2, 0.65, layer=pcbnew.Cmts_User)

    for layer in [pcbnew.F_Cu, pcbnew.B_Cu]:
        add_zone(board, nets["GND"], layer)
    pcbnew.SaveBoard(str(BOARD_PATH), board)

    # KiCad 9 can crash if a heavily edited board is zone-filled before it is
    # reloaded.  Save first, then fill zones in a fresh KiCad Python process.
    fill_script = (
        "import pcbnew; "
        f"path={str(BOARD_PATH)!r}; "
        "board=pcbnew.LoadBoard(path); "
        "pcbnew.ZONE_FILLER(board).Fill(board.Zones()); "
        "pcbnew.SaveBoard(path, board)"
    )
    subprocess.run([sys.executable, "-c", fill_script], check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
