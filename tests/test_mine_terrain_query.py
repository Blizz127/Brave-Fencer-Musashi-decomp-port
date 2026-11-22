"""Check the mine terrain query's endpoint preparation, retries and classification."""
import os
from pathlib import Path
import shlex
import subprocess


def test_mine_terrain_query(tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    exe = tmp_path / "mine-terrain-query"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2", "-Wall", "-Wextra",
        "-Werror", "-UNDEBUG", "-I", root / "include", "-I", scene,
        root / "tests/mine_terrain_query_probe.c", scene / "80133784.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
