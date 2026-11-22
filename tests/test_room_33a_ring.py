"""Exercise the phase-six callback through the room's type 33A selector."""
import os
from pathlib import Path
import shlex
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def test_room_33a_ring(tmp_path):
    overlay = ROOT / "src/overlays/sc03_0116"
    executable = tmp_path / "room_33a_ring"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2",
        "-Wall", "-Wextra", "-Werror", "-UNDEBUG", "-I", ROOT / "include",
        "-I", overlay, ROOT / "tests/room_33a_ring_probe.c",
        overlay / "801814b0.c", overlay / "8018172c.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
