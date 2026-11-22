"""Exercise both terrain clearance thresholds and signed dot-product overflow."""
import os
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[1]


def test_mine_terrain_plane_clearance(tmp_path):
    overlay = ROOT / "src/overlays/sc02_0039"
    probe = tmp_path / "mine-terrain-plane"
    subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
                    "-I", ROOT / "include", ROOT / "tests/mine_terrain_plane_probe.c",
                    overlay / "80134310.c", overlay / "8013435c.c", "-o", probe], check=True)
    subprocess.run([probe], check=True)
