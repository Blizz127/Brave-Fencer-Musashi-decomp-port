from pathlib import Path
import json
import shutil
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import native_lane_gen as lane


def test_controller_and_movement(tmp_path):
    subprocess.run([
        "cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-fno-strict-aliasing", "-I", str(ROOT / "include"),
        str(ROOT / "tests/scene_player_motion_probe.c"), "-o", str(tmp_path / "probe"),
    ], check=True)
    subprocess.run([str(tmp_path / "probe")], check=True)


@pytest.mark.parametrize("pc", sorted(lane.SC01_PLAYER_MOTION_EXTENTS))
def test_player_motion_admission(pc, tmp_path, monkeypatch):
    registry = json.loads((ROOT / "provenance/scene-objects.json").read_text())
    row = next(r for r in registry["matches"] if r["region"] == "sc01_0000" and r["vram"] == pc)
    source = ROOT / row["source"]
    size, _ = lane.SC01_PLAYER_MOTION_EXTENTS[pc]
    assert size == row["size"]
    data = bytes(size)
    info = dict(member="sc99_0001", vram=pc, start=0, code=[(pc, pc + size)], data=data)
    (tmp_path / "ir").mkdir()

    def check(path, region, member):
        return lane.build_one(dict(row, region=region, source=str(path)), tmp_path, {}, {}, "clang", member)

    assert check(source, "sc01_0000", None).get("rule") == "memory"
    copy = tmp_path / "alternative.c"
    copy.write_bytes(source.read_bytes())
    assert check(copy, "main", None).get("rule") == "memory"
    assert check(copy, "sc99_0001", info).get("rule") == "memory"

    # Exercise matching member identity separately from matching source text.
    copy.write_text(f"int {row['name']}(int input) {{ return input + 1; }}\n")
    assert check(copy, "sc01_0000", None).get("rule") == "memory"
    assert check(copy, "sc99_0001", dict(info, member="sc01_0000")).get("rule") == "memory"
    monkeypatch.setitem(lane.SC01_PLAYER_MOTION_EXTENTS, pc, (size, lane.fnv1a64(data)))
    assert check(copy, "sc99_0001", info).get("rule") == "memory"
    if not all(shutil.which(tool) for tool in ("clang", "nm")):
        pytest.skip("clang and nm required to compile an independent member")
    differing = dict(info, data=b"\x01" + data[1:])
    assert check(copy, "sc99_0001", differing)["admitted"]
    assert check(copy, "main", None)["admitted"]
