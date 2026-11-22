"""Verify that mine frame callbacks observe edits to later records and handlers."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/overlays/sc02_0039"


def test_mine_frame_slot_dispatch(tmp_path):
    exe = tmp_path / "mine-frame-slots"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", "-I", SOURCE, ROOT / "tests/mine_frame_slots_probe.c",
        SOURCE / "8016e95c.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
