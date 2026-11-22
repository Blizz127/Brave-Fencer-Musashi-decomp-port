"""Check target-visible flag and alias behavior through the port's checked RAM API."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import json


def test_sound_record_flags(tmp_path):
    root = Path(__file__).resolve().parents[1]
    program = tmp_path / "sound_record_clear"
    crypto = ([str(root / "pc_port/platform/bfm_plat_sha256.c")] if sys.platform == "win32" else ["-lcrypto"])
    subprocess.run([
        *shlex.split(os.environ.get("CC") or "cc"),
        "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
        "-I", str(root / "pc_port/include"), "-I", str(root / "include"),
        str(root / "tests/sound_record_clear_probe.c"),
        str(root / "pc_port/sound_record_clear.c"), str(root / "pc_port/boot_memory.c"),
        *crypto, "-o", str(program),
    ], check=True)
    subprocess.run([program], check=True)


def test_record_clear_timing_hold(tmp_path, monkeypatch):
    root = Path(__file__).resolve().parents[1]
    monkeypatch.syspath_prepend(str(root / "tools"))
    import native_lane_gen as lane

    pc = 0x8002cc4c
    entries = json.loads((root / "provenance/native-main.json").read_text())["matches"]
    row = next(entry for entry in entries if entry["vram"] == pc)
    source = root / row["source"]
    other = "sc99_0001"
    extent = bytes(range(104))
    prefix = bytes(8)
    image = dict(member=other, vram=pc, start=len(prefix), data=prefix + extent + bytes(8),
                 code=[(pc, pc + 112)])
    (tmp_path / "ir").mkdir()
    monkeypatch.setattr(lane, "MAIN_EXE", tmp_path / "absent")

    def check(path, region, image=None, size=104):
        candidate = dict(row, size=size, source=str(path), region=region)
        return lane.build_one(candidate, tmp_path, {}, {}, "clang", image)

    assert check(source, "main")["rule"] == "timing"
    assert check(source, other)["rule"] == "timing"
    detached = tmp_path / "independent.c"
    detached.write_bytes(source.read_bytes())
    assert check(detached, other)["rule"] == "timing"
    assert check(detached, other, image)["rule"] == "timing"
    detached.write_text("int func_8002CC4C(void) { return 7; }\n")
    assert check(detached, None)["rule"] == "timing"
    assert check(detached, "main", image)["rule"] == "timing"
    # Without the matching member this foreign source cannot be installed.
    for incomplete in (None, {}, dict(member=other), dict(vram=pc, start=0, data=b""),
                       dict(image, member="sc99_0002"), dict(image, code=[(pc, pc + 103)])):
        assert check(detached, other, incomplete)["rule"] == "overlay"
    assert check(tmp_path / "missing.c", other)["rule"] == "source"
    # Adjacent and overlapping code intervals can jointly cover a complete function.
    adjacent = [(pc + 40, pc + 104), (pc + 24, pc + 40), (pc, pc + 28)]
    assert check(detached, other, dict(image, code=adjacent))["admitted"]
    gap = [(pc + 41, pc + 104), (pc, pc + 40)]
    assert check(detached, other, dict(image, code=gap))["rule"] == "overlay"
    # Supply a synthetic fingerprint while exercising the real fixed-span reader.
    with monkeypatch.context() as aliases:
        digest = lane.fnv1a64
        aliases.setattr(lane, "fnv1a64",
                        lambda data: 0xd34ed2926778bca1 if data == extent else digest(data))
        for size in (4, 100, 104, 108):
            assert check(detached, other, image, size)["rule"] == "timing"
        assert check(detached, other, dict(image, data=prefix + extent), 4)["rule"] == "timing"
        assert check(detached, other, dict(image, start=-1), 4)["rule"] == "overlay"
        # A fingerprint requires 104 mapped code bytes, including across adjacent sections.
        assert check(detached, other, dict(image, code=adjacent), 4)["rule"] == "timing"
        assert check(detached, other, dict(image, code=gap), 4)["admitted"]
        for size in (4, 8, 100):
            limited_code = [(pc, pc + size)]
            for data in (prefix + extent, prefix + extent[:-1] + b"X"):
                assert check(detached, other, dict(image, code=limited_code, data=data), size)["admitted"]
        assert check(detached, other, dict(image, code=[]), 4)["rule"] == "overlay"
        assert check(detached, other, dict(image, vram=pc + 4, start=12), 4)["rule"] == "overlay"
        short_image = dict(image, data=prefix + extent[:-1], code=[(pc, pc + 100)])
        assert check(detached, other, short_image, 100)["admitted"]
        assert check(detached, other, short_image, 104)["rule"] == "overlay"
    independent = check(detached, other, image)
    assert independent["admitted"] and independent["member"] == other
