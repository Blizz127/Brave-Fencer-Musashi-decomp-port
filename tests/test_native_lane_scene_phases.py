"""The countdown and fade callbacks still need the guest bus and event timing."""
from pathlib import Path
import shutil
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import native_lane_gen as lane


PHASES = (
    ("sc02_0039", 0x80167AE0, 732),
    ("sc02_0039", 0x801299C8, 632),
    ("sc02_0039", 0x801287B8, 248),
    ("sc02_0039", 0x80135D20, 400),
    ("sc01_0077", 0x80135D20, 400),
    ("sc01_0077", 0x80150150, 32),
    ("main_0012", 0x801287B8, 248),
    ("main_0012", 0x8014A2E4, 156),
    ("main_0012", 0x8014C278, 56),
    ("main_0012", 0x8014FDF4, 108),
    ("main_0012", 0x80150150, 32),
    ("main_0012", 0x80150460, 32),
    ("main_0012", 0x801505FC, 168),
    ("sc02_0039", 0x801506A4, 380),
    ("main_0012", 0x801508B4, 68),
    ("sc02_0039", 0x8016EE40, 84),
    ("sc02_0039", 0x8017CDD8, 68),
    ("sc02_0039", 0x8017CE1C, 36),
    ("sc02_0039", 0x8017CE40, 64),
    ("sc02_0039", 0x8017CEFC, 32),
    ("sc02_0039", 0x8017CF58, 116),
    ("sc02_0039", 0x8017CFCC, 40),
    ("sc02_0039", 0x8017CFF4, 40),
    ("main_0010", 0x800D1EBC, 156),
)


@pytest.mark.parametrize("member,pc,size", PHASES)
def test_phase_source_stays_interpreted(member, pc, size, tmp_path, monkeypatch):
    # The forest floor wrapper uses the identical registered MAIN source.
    source_member = "main_0012" if (member, pc) == ("sc01_0077", 0x80150150) else member
    source = ROOT / "src/overlays" / source_member / f"{pc:08x}.c"
    row = dict(name=f"func_{pc:08X}", vram=pc, size=size, source=str(source), region=member)
    (tmp_path / "ir").mkdir()

    def check(entry, info=None):
        return lane.build_one(entry, tmp_path, {}, {}, "clang", info)

    assert check(row).get("rule") == "timing"
    copy = tmp_path / f"{pc:08x}.c"
    copy.write_bytes(source.read_bytes())
    alternate = dict(row, region="sc99_0001", source=str(copy))
    assert check(alternate).get("rule") == "timing"
    copy.write_text(f"int {row['name']}(int x) {{ return x + 1; }}\n")
    assert check(dict(alternate, region=member)).get("rule") == "timing"
    assert check(alternate, dict(member=member)).get("rule") == "timing"

    # Model another member with exactly the same complete guest body at this PC.
    data = bytes(size)
    info = dict(member="sc99_0001", vram=pc, start=0, code=[(pc, pc + size)], data=data)
    monkeypatch.setitem(lane.SCENE_PHASE_EXTENTS[pc], member, (size, lane.fnv1a64(data)))
    assert check(alternate, info).get("rule") == "timing"
    if not all(shutil.which(tool) for tool in ("clang", "nm")):
        pytest.skip("clang and nm required for the independent-source control")
    assert check(alternate, dict(info, data=b"\x01" + data[1:]))["admitted"]
    assert check(alternate)["admitted"]


@pytest.mark.skipif(not shutil.which("clang"), reason="clang required to inspect the dispatcher")
@pytest.mark.parametrize("pc", (0x8017CD9C, 0x8017CF1C))
def test_phase_dispatch_rejects_host_function_table(pc, tmp_path):
    source = ROOT / "src/overlays/sc02_0039" / f"{pc:08x}.c"
    row = dict(name=f"func_{pc:08X}", vram=pc, size=60, source=str(source),
               region="sc02_0039")
    (tmp_path / "ir").mkdir()
    result = lane.build_one(row, tmp_path, {}, {}, "clang")
    assert result.get("rule") == "layout"
    assert result.get("detail") == "array of pointers"

@pytest.mark.parametrize("pc, size", [
    (0x8012A908, 128), (0x80162B1C, 424), (0x8017849C, 364),
    (0x8017869C, 168), (0x801787D4, 108),
])
def test_mine_slot_dispatch_preserves_the_main_member(tmp_path, pc, size):
    mine = ROOT / f"src/overlays/sc02_0039/{pc:08x}.c"
    main = ROOT / f"src/overlays/main_0012/{pc:08x}.c"
    row = dict(region="sc02_0039", vram=pc, size=size)
    copy = tmp_path / "copy.c"
    copy.write_bytes(mine.read_bytes())
    assert lane.mine_slot_update_source_only(row, main, None)
    assert lane.mine_slot_update_source_only(dict(row, region="sc99_0001"), copy, None)
    assert lane.mine_slot_update_source_only(dict(row, region="sc99_0001"), main,
                                             dict(member="sc02_0039"))
    assert not lane.mine_slot_update_source_only(dict(row, region="main_0012"), main,
                                                 dict(member="main_0012"))
    assert not lane.mine_slot_update_source_only(dict(row, region="sc99_0001"), main, None)
