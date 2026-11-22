"""Run the forest motion transition with synthetic actors and services."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def test_forest_kind2_phase(tmp_path):
    binary = tmp_path / "kind2-phase"
    subprocess.run([
        "cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        ROOT / "tests/forest_kind2_phase_probe.c",
        ROOT / "src/overlays/sc01_0077/8015b950.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
