"""Exercise room 304C door setup, model exhaustion and the following idle phase."""
import os
from pathlib import Path
import shlex
import subprocess


def test_room_304c_door(tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc03_0116"
    exe = tmp_path / "door_15d"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2", "-Wall", "-Wextra",
        "-Werror", "-UNDEBUG", "-I", root / "include", "-I", scene,
        root / "tests/room_304c_door_probe.c", scene / "80131f64.c", scene / "80132144.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
