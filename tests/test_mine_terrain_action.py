"""Verify the mine player's terrain result, action and saved surface normal."""
import os
from pathlib import Path
import shlex
import subprocess


def test_mine_terrain_action(tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    probe = tmp_path / "mine-terrain-action"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-O2", "-Wall", "-Wextra",
        "-Werror", "-UNDEBUG", "-I", root / "include", "-I", scene,
        root / "tests/mine_terrain_action_probe.c", scene / "8014fa70.c", "-o", probe,
    ], check=True)
    subprocess.run([probe], check=True)
