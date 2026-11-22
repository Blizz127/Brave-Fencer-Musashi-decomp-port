"""Check the room event dispatcher and setup stage against synthetic service behavior."""
import os
from pathlib import Path
import shlex
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def test_room_event(tmp_path):
    overlay = ROOT / "src/overlays/sc03_0116"
    exe = tmp_path / "room_event"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2",
        "-Wall", "-Wextra", "-Werror", "-UNDEBUG", "-I", ROOT / "include",
        "-I", overlay, ROOT / "tests/room_event_probe.c",
        overlay / "80180994.c", overlay / "801809d0.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
