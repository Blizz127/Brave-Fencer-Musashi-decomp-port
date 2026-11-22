"""Exercise the member-specific door dispatch and both model acquisition paths."""
import os
from pathlib import Path
import shlex
import subprocess


def test_mine_door_phase(tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    exe = tmp_path / "door_phase"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2", "-Wall", "-Wextra",
        "-Werror", "-UNDEBUG", "-I", root / "include", "-I", scene,
        root / "tests/mine_door_phase_probe.c", scene / "8012c1b8.c",
        scene / "80131fdc.c", scene / "8013221c.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
