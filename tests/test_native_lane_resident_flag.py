"""The resident flag getter requires instruction-granular guest execution."""
import json
from pathlib import Path
import shutil
import sys

import pytest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import native_lane_gen as lane


def test_flag_getter_identity(tmp_path, monkeypatch):
    pc = 0x80029178
    registry = json.loads((ROOT / "provenance/native-main.json").read_text())
    row = next(entry for entry in registry["matches"] if entry["vram"] == pc)
    source = ROOT / row["source"]
    other = "sc99_0001"
    info = dict(member=other, vram=pc, start=0, data=bytes(40), code=[(pc, pc + 40)])
    (tmp_path / "ir").mkdir()
    monkeypatch.setattr(lane, "MAIN_EXE", tmp_path / "absent")

    def check(path, region, image=None):
        return lane.build_one(dict(row, region=region, source=str(path)),
                              tmp_path, {}, {}, "clang", image)

    assert check(source, "main")["rule"] == "timing"
    assert check(source, other, info)["rule"] == "timing"
    copy = tmp_path / "independent.c"
    copy.write_bytes(source.read_bytes())
    assert check(copy, other, info)["rule"] == "timing"
    copy.write_text("int func_80029178(unsigned index) { return index & 1; }\n")
    assert check(copy, "main")["rule"] == "timing"
    assert check(copy, "main", info)["rule"] == "timing"
    # Only the fingerprint is substituted. No target bytes are part of this fixture.
    with monkeypatch.context() as aliases:
        aliases.setattr(lane, "member_digest", lambda image, address, size: 0xe209fbd89c4aa62b)
        assert check(copy, other, info)["rule"] == "timing"
    if not all(shutil.which(tool) for tool in ("clang", "nm")):
        pytest.skip("clang and nm required to build an independent overlay")
    independent = check(copy, other, info)
    assert independent["admitted"], independent
    assert independent["member"] == other


def test_resident_flag_caller_uses_guest_thunk(tmp_path, monkeypatch):
    if not all(shutil.which(tool) for tool in ("clang", "nm")):
        pytest.skip("clang and nm required to generate the caller")
    (tmp_path / "ir").mkdir()
    monkeypatch.setattr(lane, "MAIN_EXE", tmp_path / "absent")
    rows = json.loads((ROOT / "provenance/native-main.json").read_text())["matches"]
    # The matched resident caller actually uses this flag and two other services.
    both = [lane.build_one(row, tmp_path, {}, {}, "clang")
            for row in rows if row["vram"] in (0x80029178, 0x80029DB4)]
    assert [(row["vram"], row["admitted"]) for row in both] == [
        (0x80029178, False), (0x80029DB4, True)]
    direct, thunks = lane.emit_table([row for row in both if row["admitted"]], tmp_path)
    table = (tmp_path / "native_lane_table.c").read_text()
    assert (direct, thunks) == (0, 3)
    assert "musashi_native_guest_call(0x80029178u, 1u, args, NULL)" in table
    assert "wrap_80029178" not in table
    assert "lane_func_80029178(" not in table
