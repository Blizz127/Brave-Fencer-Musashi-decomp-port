"""The controller report reset retains per-store timing and return register state."""
import json
from pathlib import Path
import shutil
import sys

import pytest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import native_lane_gen as lane


def test_indexed_reset_cannot_become_an_atomic_wrapper(tmp_path, monkeypatch):
    rows = json.loads((ROOT / "provenance/native-main.json").read_text())["matches"]
    reset = next(row for row in rows if row["vram"] == 0x80018F88)
    path = tmp_path / "reset.c"
    path.write_text(
        '#include "controller_report.h"\n'
        'void func_80018F88(PadRecord *r) {\n'
        '    int i;\n'
        '    for (i = 1; i >= 0; --i) r->current[i] = 0;\n'
        '    for (i = 2; i < 6; ++i) r->current[i] = 128;\n'
        '}\n'
    )
    (tmp_path / "ir").mkdir()
    monkeypatch.setattr(lane, "MAIN_EXE", tmp_path / "absent")
    row = dict(reset, source=str(path))
    result = lane.build_one(row, tmp_path, {}, {}, "clang")
    assert result["rule"] == "timing"
    other = "sc99_0001"
    image = dict(member=other, vram=row["vram"], start=0, data=bytes(64),
                 code=[(row["vram"], row["vram"] + 64)])
    row["region"] = other
    with monkeypatch.context() as alias:
        alias.setattr(lane, "member_digest", lambda image, pc, size: 0x102b93aa2fe67a66)
        assert lane.build_one(row, tmp_path, {}, {}, "clang", image)["rule"] == "timing"
    if not all(shutil.which(tool) for tool in ("clang", "nm")):
        pytest.skip("clang and nm are required to compile the unrelated overlay")
    independent = lane.build_one(row, tmp_path, {}, {}, "clang", image)
    assert independent["admitted"], independent
