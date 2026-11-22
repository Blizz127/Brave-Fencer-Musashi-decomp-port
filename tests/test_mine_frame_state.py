"""Check per-frame mine state dispatch and the timer-dependent mode flag."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/overlays/sc02_0039"


def test_mine_frame_state_callbacks(tmp_path):
    exe = tmp_path / "mine-frame-state"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", "-I", SOURCE, ROOT / "tests/mine_frame_state_probe.c",
        SOURCE / "801627e8.c", SOURCE / "801754a8.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
