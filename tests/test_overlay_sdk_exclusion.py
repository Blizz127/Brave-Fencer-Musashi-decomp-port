"""Member-specific exclusion of the intro movie overlay's SDK code."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import native_lane_gen as lane
import psyq_manifest as sdk


def test_libpress_source_boundaries():
    table = sdk.ranges()
    for pc, expected in (
        (0x800D2A24, None), (0x800D2A9F, None), (0x800D2AA0, "libpress"),
        (0x800D31FF, "libpress"), (0x800D3200, None), (0x800D3203, None),
        (0x800D3204, "libpress"), (0x800D3573, "libpress"), (0x800D3574, None),
    ):
        assert sdk.source_segment(f"src/overlays/main_0003/{pc:08x}.c", table) == expected
        assert sdk.source_segment(f"src/overlays/main_0010/{pc:08x}.c", table) is None
        assert sdk.source_segment(f"src/main/{pc:08x}.c", table) is None
    assert sdk.check([r"src\overlays\main_0003\800d2aa0.c"], None) == 1
    assert sdk.check(["$<1:src/overlays/main_0003/800d2aa0.c>"], None) == 1
    assert sdk.check(["src/overlays/main_0010/800d2aa0.c"], None) == 0


def test_libpress_overlay_selection(tmp_path):
    source = tmp_path / "game.c"
    source.write_text("int game(void) { return 1; }\n")
    pc = 0x800D2AA0
    records = [dict(region=member, name="game", recovery="c", vram=pc,
                    size=4, source=str(source)) for member in ("main_0003", "main_0010")]
    before = dict(records[0], vram=pc - 4)
    crossing = dict(before, size=8)
    rows = [*records, before, crossing]
    candidates = lane.overlay_candidates({"main_0003", "main_0010"}, registry=rows)
    assert candidates == [records[1], before]
    for row, info in ((records[0], None), (records[1], {"member": "main_0003"}),
                      (crossing, {"member": "main_0003"})):
        result = lane.build_one(row, tmp_path, {}, {}, "clang", info)
        assert result["rule"] == "deny"
