"""Run both forest animation routines with synthetic scripts and actors."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def test_forest_animation_commands(tmp_path):
    binary = tmp_path / "forest-animation-commands"
    scene = ROOT / "src/overlays/sc01_0077"
    subprocess.run([
        "cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        ROOT / "tests/forest_animation_commands_probe.c", scene / "8015444c.c",
        scene / "801549f8.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
