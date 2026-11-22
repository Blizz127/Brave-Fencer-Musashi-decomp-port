"""Check the eight room actor stages and the phase-zero initializer through their public C entry points."""
import os
from pathlib import Path
import shlex
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def test_room_33a_stage(tmp_path):
    overlay = ROOT / "src/overlays/sc03_0116"
    exe = tmp_path / "room_33a_stage"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2",
        "-Wall", "-Wextra", "-Werror", "-UNDEBUG", "-I", ROOT / "include",
        "-I", overlay, ROOT / "tests/room_33a_stage_probe.c",
        overlay / "801814b0.c", overlay / "801814ec.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
