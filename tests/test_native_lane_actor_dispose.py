"""The Tower's actor cleanup and its historical callers retain guest execution."""
import hashlib
from pathlib import Path
import shutil
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import native_lane_gen as lane


@pytest.mark.parametrize("region", ["sc02_0035", "main_0012"])
def test_tower_dispose_and_foreign_source_at_shared_address(region, tmp_path):
    pc = 0x8012CAE4
    source = ROOT / "src/overlays/sc02_0005/8012cae4.c"
    copied = tmp_path / "renamed.c"
    copied.write_bytes(source.read_bytes())
    row = dict(name="func_8012CAE4", region=region, vram=pc, size=128, source=str(copied))
    info = dict(member=region, vram=pc, start=0, code=[(pc, pc + 128)], data=bytes(128))
    (tmp_path / "ir").mkdir()
    assert lane.build_one(row, tmp_path, {}, {}, "clang", info)["rule"] == "timing"
    assert lane.build_one(row, tmp_path, {}, {}, "clang")["rule"] == "timing"
    copied.write_text("int func_8012CAE4(int value) { return value + 1; }\n")
    assert lane.build_one(dict(row, region="sc02_0005"), tmp_path, {}, {}, "clang", info)["rule"] == "timing"
    assert lane.build_one(row, tmp_path, {}, {}, "clang", dict(info, member="sc02_0005"))["rule"] == "timing"
    if not all(shutil.which(tool) for tool in ("clang", "nm")):
        pytest.skip("clang and nm required for the independent-source control")
    assert lane.SCENE_PHASE_EXTENTS[pc] == {"sc02_0005": None}
    assert lane.build_one(row, tmp_path, {}, {}, "clang", info)["admitted"]


@pytest.mark.parametrize("pc,size", [
    (0x8012C354, 228), (0x80184EC0, 416), (0x8018968C, 616), (0x8018BA38, 172),
])
def test_earlier_tower_caller_marker_is_specific_to_its_pc(pc, size, tmp_path, monkeypatch):
    if not all(shutil.which(tool) for tool in ("clang", "nm")):
        pytest.skip("clang and nm required for the independent-source control")
    other_pc = 0x8018968C if pc == 0x8012C354 else 0x8012C354
    source = tmp_path / "renamed.c"
    source.write_text(f"int func_{pc:08X}(int value) {{ return value + 1; }}\n")
    row = dict(name=f"func_{pc:08X}", vram=pc, size=size, region="sc02_0035", source=str(source))
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    (tmp_path / "ir").mkdir()
    for address in [other_pc, pc]:
        assert lane.TOWER_EARLIER_CALLERS[address]
        monkeypatch.setitem(lane.TOWER_EARLIER_CALLERS, address, lane.TOWER_EARLIER_CALLERS[address] | {digest})
        result = lane.build_one(row, tmp_path, {}, {}, "clang")
        if address == pc:
            assert result["rule"] == "timing"
        else:
            assert result["admitted"]
