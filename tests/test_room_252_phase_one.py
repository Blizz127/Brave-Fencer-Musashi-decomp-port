"""Exercise the room actor stage-one light through the matching C function."""
import os
from pathlib import Path
import shlex
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def test_room_252_phase_one(tmp_path):
    overlay = ROOT / "src/overlays/sc03_0116"
    exe = tmp_path / "room_252_phase_one"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2",
        "-Wall", "-Wextra", "-Werror", "-UNDEBUG", "-I", ROOT / "include",
        "-I", overlay, ROOT / "tests/room_252_phase_one_probe.c",
        overlay / "80182378.c", overlay / "801825a8.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
