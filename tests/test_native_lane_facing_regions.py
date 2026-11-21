"""The scene facing routines require the guest bus and their own clock contract."""
import json
from pathlib import Path
import shutil
import sys

import pytest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import native_lane_gen as lane


@pytest.mark.parametrize("pc", (0x8017D484, 0x8017D5F4, 0x8017D6AC))
def test_facing_region_admission(pc, tmp_path, monkeypatch):
    registry = json.loads((ROOT / "provenance/scene-objects.json").read_text())
    row = next(r for r in registry["matches"] if r["vram"] == pc and r["region"] == "sc02_0039")
    size = row["size"]
    data = bytes(size)
    info = dict(member="sc99_0001", vram=pc, start=0, code=[(pc, pc + size)], data=data)
    (tmp_path / "ir").mkdir()

    def check(source, member):
        return lane.build_one(dict(row, source=str(source)), tmp_path, {}, {}, "clang", member)

    source = ROOT / row["source"]
    assert check(source, None).get("rule") == "memory"
    assert check(source, dict(info, member="sc02_0039")).get("rule") == "memory"
    other = tmp_path / "other.c"
    other.write_bytes(source.read_bytes())
    assert check(other, None).get("rule") == "memory"

    # Use synthetic member bytes to exercise the same extent identity check.
    other.write_text(f"int {row['name']}(int x) {{ return x + 1; }}\n")
    assert lane.FACING_EXTENTS[pc][0] == size
    monkeypatch.setitem(lane.FACING_EXTENTS, pc, (size, lane.fnv1a64(data)))
    assert check(other, info).get("rule") == "memory"
    if not all(shutil.which(tool) for tool in ("clang", "nm")):
        pytest.skip("clang and nm required for the independent overlay check")
    different = dict(info, data=b"\x01" + data[1:])
    assert check(other, different)["admitted"]
    assert check(other, None)["admitted"]
