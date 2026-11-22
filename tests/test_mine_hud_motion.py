"""Exercise the mine number display's signed setter and three drawing phases."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/overlays/sc02_0039"


def test_mine_hud_motion_callbacks(tmp_path):
    exe = tmp_path / "mine-hud-motion"
    sources = [SOURCE / name for name in ("8016e918.c", "8016eac0.c", "8016eb3c.c", "8016eba8.c")]
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", "-I", SOURCE, ROOT / "tests/mine_hud_motion_probe.c",
        *sources, "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
