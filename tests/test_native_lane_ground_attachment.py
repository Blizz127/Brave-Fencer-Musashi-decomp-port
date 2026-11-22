"""Keep unqualified forest controller and orientation callbacks on the guest path."""

import hashlib
from pathlib import Path
import shutil
import sys

import pytest


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import native_lane_gen as lane


CASES = [
    ("sc01_0077", 0x801627E8, 76),
    ("sc01_0077", 0x80162834, 60),
    ("main_0012", 0x801628F4, 116),
    ("main_0012", 0x80162968, 344),
]


@pytest.mark.parametrize("member,pc,size", CASES)
def test_ground_attachment_selection(member, pc, size, tmp_path, monkeypatch):
    source = ROOT / "src" / "overlays" / member / f"{pc:08x}.c"
    row = dict(vram=pc, size=size, name=f"func_{pc:08X}")
    detached = tmp_path / "field.c"
    detached.write_bytes(source.read_bytes())
    (tmp_path / "ir").mkdir()

    def selected(file, region, info=None):
        return lane.build_one(dict(row, region=region, source=str(file)),
                              tmp_path, {}, {}, "clang", info)

    assert selected(source, member)["rule"] == "timing"
    other = "sc99_0001"
    with monkeypatch.context() as isolated:
        isolated.setattr(lane, "ROOT", tmp_path)
        assert selected(detached, other)["rule"] == "timing"

    independent = f"int {row['name']}(int x) {{ return x + 1; }}\n".encode()
    detached.write_bytes(independent)
    assert selected(detached, member)["rule"] == "timing"
    assert selected(detached, other, dict(member=member))["rule"] == "timing"

    # A separately relocated copy of the historical C can lack all member data.
    with monkeypatch.context() as historical:
        historical.setitem(lane.FOREST_GROUND_ATTACHMENT_SOURCES, pc,
                           {hashlib.sha256(independent).hexdigest()})
        assert selected(detached, other)["rule"] == "timing"

    # A known complete code extent identifies the game function after relocation.
    fixture = bytes((i * 17) & 0xff for i in range(size))
    info = dict(member=other, vram=pc, start=0, data=fixture, code=[(pc, pc + size)])
    monkeypatch.setitem(lane.FOREST_GROUND_ATTACHMENT_EXTENTS[pc],
                        member, (size, lane.fnv1a64(fixture)))
    assert selected(detached, other, info)["rule"] == "timing"
    for supplied_size in (size - 4, size + 4):
        with monkeypatch.context() as incomplete:
            incomplete.setitem(row, "size", supplied_size)
            assert selected(detached, other, info)["rule"] == "timing"
    if not all(shutil.which(name) for name in ("clang", "nm")):
        pytest.skip("clang and nm are required to compile independent member controls")
    distinct = dict(info, data=fixture[:-1] + bytes([fixture[-1] ^ 1]))
    assert selected(detached, other, distinct)["admitted"]
    assert selected(detached, other)["admitted"]
