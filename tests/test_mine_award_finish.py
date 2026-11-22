"""Exercise the mine player's award controller return turning and final idle phases."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/overlays/sc02_0039"


def test_mine_award_finish_callbacks(tmp_path):
    exe = tmp_path / "mine-award-finish"
    sources = [SOURCE / name for name in ("801741ec.c", "80174274.c")]
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", "-I", SOURCE, ROOT / "tests/mine_award_finish_probe.c",
        *sources, "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
