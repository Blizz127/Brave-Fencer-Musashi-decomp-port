"""Exercise the mine player's award controller initial state, selector and optional movement phases."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/overlays/sc02_0039"


def test_mine_award_early_callbacks(tmp_path):
    exe = tmp_path / "mine-award-early"
    sources = [SOURCE / name for name in ("80173b88.c", "80173bc0.c", "80173c64.c")]
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", "-I", SOURCE, ROOT / "tests/mine_award_early_probe.c",
        *sources, "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
