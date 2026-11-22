"""Check mine backdrop gradients, scratch transfers, and submission timing."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/overlays/sc02_0039"


def test_mine_backdrop_submission(tmp_path):
    exe = tmp_path / "mine-backdrop"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", "-I", SOURCE, ROOT / "tests/mine_backdrop_probe.c",
        SOURCE / "8012956c.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
