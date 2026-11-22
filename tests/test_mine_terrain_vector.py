"""Check the mine vector query's modes, output writes and collision flags."""
import os
from pathlib import Path
import shlex
import subprocess


def test_mine_terrain_vector(tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    executable = tmp_path / "mine-terrain-vector"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2",
        "-Wall", "-Wextra", "-Werror", "-UNDEBUG", "-I", root / "include", "-I", scene,
        root / "tests/mine_terrain_vector_probe.c", scene / "8012ceb0.c", "-o", executable,
    ], check=True)
    subprocess.run([executable], check=True)
