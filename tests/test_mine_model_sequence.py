"""Exercise the mine model sequence's bound pointers, action flags and recursive returns."""
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "src/overlays/sc02_0039"


def test_mine_model_sequence_commands(tmp_path):
    exe = tmp_path / "mine-model-sequence"
    subprocess.run([
        os.environ.get("CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-I", ROOT / "include", "-I", SOURCE, ROOT / "tests/mine_model_sequence_probe.c",
        SOURCE / "8015444c.c", "-o", exe,
    ], check=True)
    subprocess.run([exe], check=True)
