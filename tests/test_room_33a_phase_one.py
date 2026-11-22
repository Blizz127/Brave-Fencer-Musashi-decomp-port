"""Compile and exercise the room actor's phase-one transition and its selector."""
import os
from pathlib import Path
import shlex
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def test_room_33a_phase_one(tmp_path):
    overlay = ROOT / "src/overlays/sc03_0116"
    exe = tmp_path / "room_33a_phase_one"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2",
        "-Wall", "-Wextra", "-Werror", "-UNDEBUG", "-I", ROOT / "include",
        "-I", overlay, ROOT / "tests/room_33a_phase_one_probe.c",
        overlay / "801814b0.c", overlay / "80181550.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
