"""Exercise the mine's cell lookup and ordered terrain-face callbacks."""
import os
from pathlib import Path
import shlex
import subprocess


def test_mine_terrain_cell(tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    probe = tmp_path / "mine-terrain-cell"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2", "-Wall", "-Wextra",
        "-Werror", "-UNDEBUG", "-I", root / "include", "-I", scene,
        root / "tests/mine_terrain_cell_probe.c", scene / "80133ab0.c", "-o", probe,
    ], check=True)
    subprocess.run([probe], check=True)
