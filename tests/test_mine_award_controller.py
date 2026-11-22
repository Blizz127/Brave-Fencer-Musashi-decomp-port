"""Exercise the mine player's award controller setup and returning phase dispatch."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/overlays/sc02_0039"


def test_mine_award_controller_callbacks(tmp_path):
    exe = tmp_path / "mine-award-controller"
    sources = [SOURCE / name for name in ("80173a28.c", "80173a60.c", "80173b4c.c")]
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", "-I", SOURCE, ROOT / "tests/mine_award_controller_probe.c",
        *sources, "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
