"""Exercise a room change and its guarded floor-grid relocation."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def test_mine_grid_binding_and_relocation(tmp_path):
    exe = tmp_path / "mine-grid-binding"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", ROOT / "tests/mine_grid_binding_probe.c",
        ROOT / "src/overlays/sc02_0039/801336e8.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
