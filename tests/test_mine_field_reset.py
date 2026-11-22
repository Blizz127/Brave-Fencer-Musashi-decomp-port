"""Verify both clears and the state visible to the placement walker."""
import os
from pathlib import Path
import shlex
import subprocess


def test_mine_field_reset(tmp_path):
    root = Path(__file__).resolve().parents[1]
    scene = root / "src/overlays/sc02_0039"
    exe = tmp_path / "mine_field_reset"
    subprocess.run([
        *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-Wall", "-Wextra",
        "-Werror", "-O2", "-UNDEBUG", "-I", root / "include", "-I", scene,
        root / "tests/mine_field_reset_probe.c", scene / "80178608.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
