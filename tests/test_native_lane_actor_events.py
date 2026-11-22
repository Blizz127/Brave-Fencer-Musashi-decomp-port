"""Actor event dispatch requires timed guest accesses to the player and event pool."""
import hashlib
from pathlib import Path
import shutil
import sys

import pytest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import native_lane_gen as lane


EVENTS = (
    ("main_0012", 0x80146750, 188),
    ("main_0012", 0x8014680C, 188),
    ("sc01_0077", 0x801498E0, 116),
    ("sc01_0077", 0x80149AA8, 44),
    ("sc01_0077", 0x80149B54, 88),
    ("sc01_0077", 0x8014BEF8, 32),
)


@pytest.mark.parametrize("member,pc,size", EVENTS)
def test_event_source_and_member_identity(member, pc, size, tmp_path, monkeypatch):
    source = ROOT / "src/overlays" / member / f"{pc:08x}.c"
    row = dict(region=member, vram=pc, size=size, name=f"func_{pc:08X}", source=str(source))
    other = "sc99_0001"
    copy = tmp_path / "relocated.c"
    copy.write_bytes(source.read_bytes())
    (tmp_path / "ir").mkdir()

    def check(file, region, info=None):
        return lane.build_one(dict(row, source=str(file), region=region), tmp_path, {}, {}, "clang", info)

    assert check(source, member)["rule"] == "timing"
    assert check(copy, other)["rule"] == "timing"
    # A partial checkout may contain only the detached C and its registry.
    with monkeypatch.context() as detached:
        detached.setattr(lane, "ROOT", tmp_path)
        assert check(copy, other)["rule"] == "timing"

    # A separately supplied registry may omit the member or declare it incorrectly.
    copy.write_text(f"int {row['name']}(int x) {{ return x + 1; }}\n")
    assert check(copy, member)["rule"] == "timing"
    assert check(copy, other, dict(member=member))["rule"] == "timing"
    independent = copy.read_bytes()
    monkeypatch.setitem(lane.FOREST_EVENT_SOURCE_HASHES, pc,
                        {hashlib.sha256(independent).hexdigest()})
    assert check(copy, other)["rule"] == "timing"
    copy.write_bytes(independent + b"\n")

    # Use synthetic bytes to test exact whole-body matching in another room.
    origin = pc - 8
    extent = bytes(range(size))
    info = dict(member=other, vram=origin, start=4, data=bytes(12) + extent,
                code=[(pc, pc + size)])
    monkeypatch.setitem(lane.FOREST_EVENT_EXTENTS[pc], member, (size, lane.fnv1a64(extent)))
    assert check(copy, other, info)["rule"] == "timing"
    if not all(shutil.which(tool) for tool in ("clang", "nm")):
        pytest.skip("clang and nm required for independent member admission")
    different = dict(info, data=info["data"][:-1] + b"\xff")
    assert check(copy, other, different)["admitted"]
    assert check(copy, other)["admitted"]


def test_resident_flag_reader_is_separate(tmp_path):
    source = tmp_path / "flag.c"
    source.write_text("unsigned char independent(unsigned char value) { return value; }\n")
    assert not lane.forest_event_source_only(dict(vram=0x80029178, size=28), source, None)
