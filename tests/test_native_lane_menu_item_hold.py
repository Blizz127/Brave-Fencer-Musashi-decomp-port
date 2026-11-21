"""The card-menu matching source stays outside the player lane."""
import hashlib
import json
from pathlib import Path
import shutil
import sys

import pytest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import native_lane_gen as lane


def test_menu_item_source_hold(tmp_path):
    manifest = json.loads((ROOT / "provenance/main12-menu-item.json").read_text())
    row = manifest["matches"][0]
    source = ROOT / row["source"]
    assert hashlib.sha256(source.read_bytes()).hexdigest() == row["source_sha256"]
    pc = row["vram"]
    # This member image and the unrelated namesake below are synthetic.
    info = dict(member="main_0012", vram=pc, start=0, code=[(pc, pc+132)], data=bytes(132))
    (tmp_path / "ir").mkdir()

    def check(file, member):
        return lane.build_one(dict(row, source=str(file)), tmp_path, {}, {}, "clang", member)

    assert check(source, info)["rule"] == "deny"
    assert check(source, None)["rule"] == "deny"
    other = dict(info, member="sc99_0001")
    assert check(source, other)["rule"] == "deny"
    copy = tmp_path / "namesake.c"
    copy.write_bytes(source.read_bytes())
    assert check(copy, None)["rule"] == "deny"
    assert check(copy, other)["rule"] == "deny"

    if not all(shutil.which(tool) for tool in ("clang", "nm")):
        pytest.skip("clang and nm required for the independent overlay check")
    copy.write_text("int func_8017CEB8(void) { return 7; }\n")
    assert check(copy, info)["rule"] == "deny"
    result = check(copy, other)
    assert result["admitted"], result
    assert result["qname"] == "sc99_0001__func_8017CEB8"
